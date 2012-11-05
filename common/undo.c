/* Copyright 2000 Kjetil S. Matheussen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */



#include "nsmtracker.h"
#include "list_proc.h"
#include "vector_proc.h"
#include "windows_proc.h"
#include "visual_proc.h"
#include "wblocks_proc.h"
#include "player_pause_proc.h"
#include "player_proc.h"

#define RADIUM_UNDOISCALLINGNOW
#include "undo.h"
#undef RADIUM_UNDOISCALLINGNOW

extern struct Root *root;

struct UndoEntry{
  NInt windownum;
  NInt blocknum;
  NInt tracknum;
  int realline;
  struct Patch *current_patch;

  void *pointer;
  UndoFunction function;
};


struct Undo{
  struct Undo *prev;
  struct Undo *next;

  vector_t entries;

  // Used to find cursorpos.
  NInt windownum;
  NInt blocknum;
  NInt tracknum;
  int realline;
};

static struct Undo UndoRoot={0};
static struct Undo *CurrUndo=&UndoRoot;
static struct Undo *curr_open_undo=NULL;

int num_undos=0;
static int max_num_undos=MAX_NUM_UNDOS;

static bool undo_is_open=false;

extern struct Patch *g_currpatch;

static bool currently_undoing = false;

void ResetUndo(void){
  if(currently_undoing){
    RError("Can not call ResetUndo from Undo()\n");
  }

  CurrUndo=&UndoRoot;
  num_undos=0;
  OS_GFX_NumUndosHaveChanged(0,false);
}

void Undo_Open(void){
  if(currently_undoing){
    RError("Can not call Undo_Open from Undo()\n");
  }

  struct Tracker_Windows *window = root->song->tracker_windows;
  struct WBlocks *wblock = window->wblock;
  struct WTracks *wtrack = wblock->wtrack;
  int realline = wblock->curr_realline;

  printf("Undo_Open\n");

  if(undo_is_open==true)
    RError("Undo_Open: Undo_is_open==true");

  curr_open_undo = talloc(sizeof(struct Undo));
  curr_open_undo->windownum=window->l.num;
  curr_open_undo->blocknum=wblock->l.num;
  curr_open_undo->tracknum=wtrack->l.num;
  curr_open_undo->realline=realline;
    

#if 0 // Disabled. Code doesn't look right.
  if(num_undos!=0 && num_undos>max_num_undos){
    num_undos--;
    UndoRoot.next=UndoRoot.next->next;
    UndoRoot.next->prev=&UndoRoot;
  }
#endif

  undo_is_open = true;
}

void Undo_Close(void){
  if(currently_undoing){
    RError("Can not call Undo_Close from Undo()\n");
  }

  undo_is_open = false;

  struct Undo *undo = curr_open_undo;

  if(undo->entries.num_elements > 0){
    undo->prev=CurrUndo;
    CurrUndo->next=undo;
    CurrUndo=undo;
    
    num_undos++;

    OS_GFX_NumUndosHaveChanged(num_undos, CurrUndo->next!=NULL);
  }
}

void Undo_CancelLastUndo(void){
  if(currently_undoing){
    RError("Can not call Undo_CancelLastUndo from Undo()\n");
  }

  CurrUndo=CurrUndo->prev;
  CurrUndo->next=NULL;
}

/***************************************************
  FUNCTION
    Insert a new undo-element.
***************************************************/
void Undo_Add(
              int windownum,
              int blocknum,
              int tracknum,
              int realline,
              void *pointer,
              UndoFunction undo_function
){
  if(currently_undoing){
    RError("Can not call Undo_Add from Undo()\n");
  }

  bool undo_was_open = undo_is_open;

  if(undo_is_open==false)
    Undo_Open();

  struct UndoEntry *entry=talloc(sizeof(struct UndoEntry));
  entry->windownum=windownum;
  entry->blocknum=blocknum;
  entry->tracknum=tracknum;
  entry->realline=realline;
  entry->current_patch = g_currpatch;
  entry->pointer=pointer;
  entry->function=undo_function;

  VECTOR_push_back(&curr_open_undo->entries,entry);

  if(undo_was_open==false)
    Undo_Close();
}

void Undo(void){
	struct Undo *undo=CurrUndo;

	if(undo==&UndoRoot) return;

currently_undoing = true;
	PlayStop();


        struct Patch *current_patch = NULL;

       {
          int i;
          for(i=undo->entries.num_elements-1 ; i>=0 ; i--){

            struct UndoEntry *entry=undo->entries.elements[i];
 
            struct Tracker_Windows *window=ListFindElement1(&root->song->tracker_windows->l,entry->windownum);
            struct WBlocks *wblock=ListFindElement1_r0(&window->wblocks->l,entry->blocknum);
            struct WTracks *wtrack=NULL;
            current_patch = entry->current_patch;

            if(wblock!=NULL){
              window->wblock=wblock;
              if(entry->tracknum<0){
                wtrack=wblock->wtracks;
              }else{
                wtrack=ListFindElement1_r0(&wblock->wtracks->l,entry->tracknum);
              }
              if(wtrack!=NULL){
                wblock->wtrack=wtrack;
              }
              wblock->curr_realline=entry->realline;
              window->curr_track=entry->tracknum;
            }

            entry->pointer = entry->function(window,wblock,wtrack,entry->realline,entry->pointer);

            wblock=ListFindElement1_r0(&window->wblocks->l,entry->blocknum);
            if(wblock==NULL)
              wblock=ListFindElement1_r0(&window->wblocks->l,entry->blocknum-1);
            if(wblock==NULL){
              RError("undo.c: block %d does not exist. Using block 0.",entry->blocknum-1);
              wblock=window->wblocks;
            }
          
            window->wblock = wblock;
            if(entry->tracknum<0){
              wtrack=wblock->wtracks;
            }else{
              wtrack=ListFindElement1_r0(&wblock->wtracks->l,entry->tracknum);
            }
            wblock->wtrack=wtrack;
            wblock->curr_realline=entry->realline;
            window->curr_track=entry->tracknum;
          }
       }

       VECTOR_reverse(&undo->entries);

       CurrUndo=undo->prev;

       num_undos--;

       {
         struct Tracker_Windows *window=ListFindElement1(&root->song->tracker_windows->l,undo->windownum);
         struct WBlocks *wblock=ListFindElement1_r0(&window->wblocks->l,undo->blocknum);
         struct WTracks *wtrack;

         if(wblock==NULL)
           wblock=window->wblocks;

         if(undo->tracknum<0){
           wtrack=wblock->wtracks;
         }else{
           wtrack=ListFindElement1_r0(&wblock->wtracks->l,undo->tracknum);
         }

         if(wtrack==NULL)
           wtrack=wblock->wtracks;
           
         wblock->wtrack=wtrack;
         wblock->curr_realline=undo->realline;
         window->curr_track=undo->tracknum;

         SelectWBlock(
                      window,
                      wblock
                      );

         if(current_patch!=NULL)
           GFX_update_instrument_patch_gui(current_patch);

       }
currently_undoing = false;

 OS_GFX_NumUndosHaveChanged(num_undos, CurrUndo->next!=NULL);
}


void Redo(void){
  printf("CurrUndo->next: %p\n",CurrUndo->next);

	if(CurrUndo->next==NULL) return;

	CurrUndo=CurrUndo->next;
	Undo();
	CurrUndo=CurrUndo->next;

	num_undos+=2;

        OS_GFX_NumUndosHaveChanged(num_undos, CurrUndo->next!=NULL);
}

void SetMaxUndos(struct Tracker_Windows *window){
	int newmax=0;
	char seltext[50];

	sprintf(seltext,"Max Undos (0=unlimited) (now: %d): ",max_num_undos);
	while(newmax==1 || newmax==2)
		newmax=GFX_GetInteger(window,NULL,seltext,0,2000000);
	if(newmax==-1) return;

	max_num_undos=newmax;

        RWarning("The max number of undoes variables is ignored. Undo is unlimited.");
}





