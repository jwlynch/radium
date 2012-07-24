// This file is automatically generated from gfx_op_queue.scm 

#ifndef VISUAL_OP_QUEUE_PROC_H 
#define VISUAL_OP_QUEUE_PROC_H 

extern LANGSPEC void OS_GFX_C2V_bitBlt(struct Tracker_Windows* window,int from_x1,int from_x2,int to_y); 
extern LANGSPEC void OS_GFX_C_DrawCursor(struct Tracker_Windows* window,int x1,int x2,int x3,int x4,int height,int y_pixmap); 
extern LANGSPEC void OS_GFX_P2V_bitBlt(struct Tracker_Windows* window,int from_x,int from_y,int to_x,int to_y,int width,int height); 
extern LANGSPEC void OS_GFX_P_FilledBox(struct Tracker_Windows* tvisual,int color,int x,int y,int x2,int y2); 
extern LANGSPEC void OS_GFX_P_Box(struct Tracker_Windows* tvisual,int color,int x,int y,int x2,int y2); 
extern LANGSPEC void OS_GFX_P_Line(struct Tracker_Windows* tvisual,int color,int x,int y,int x2,int y2); 
extern LANGSPEC void OS_GFX_P_Text(struct Tracker_Windows* tvisual,int color,char* text,int x,int y,int width,int flags); 
extern LANGSPEC void OS_GFX_Line(struct Tracker_Windows* tvisual,int color,int x,int y,int x2,int y2); 
extern LANGSPEC void OS_GFX_Box(struct Tracker_Windows* tvisual,int color,int x,int y,int x2,int y2); 
extern LANGSPEC void OS_GFX_FilledBox(struct Tracker_Windows* tvisual,int color,int x,int y,int x2,int y2); 
extern LANGSPEC void OS_GFX_Text(struct Tracker_Windows* tvisual,int color,char* text,int x,int y,int width,int flags); 
extern LANGSPEC void OS_GFX_BitBlt(struct Tracker_Windows* tvisual,int dx,int dy,int x,int y,int x2,int y2); 

#endif 
