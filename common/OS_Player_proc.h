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

//extern LANGSPEC void PLAYER_acquire_same_priority(void);
//extern LANGSPEC void PLAYER_drop_same_priority(void);

// PLAYER_lock is a RT-safe lock. Priority of the calling thread is set to the same value as the player before acquiring the lock.
// Warning, PLAYER_lock will deadlock if called from the player thread.
extern LANGSPEC void PLAYER_lock(void);
extern LANGSPEC void PLAYER_unlock(void);

extern LANGSPEC void StartPlayer(void);

extern LANGSPEC void StopPlayer(void);

extern LANGSPEC void PausePlayer(void);
extern LANGSPEC void StopPausePlayer(void);

extern LANGSPEC void OS_WaitForAShortTime(int milliseconds);



