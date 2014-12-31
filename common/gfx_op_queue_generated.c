// This file is automatically generated from gfx_op_queue.scm 

#ifdef OP_ENUMS 
ENUM_GFX_C2V_bitBlt, 
ENUM_GFX_C_DrawCursor, 
ENUM_GFX_P2V_bitBlt, 
ENUM_GFX_FilledBox, 
ENUM_GFX_Box, 
ENUM_GFX_SetClipRect, 
ENUM_GFX_CancelClipRect, 
ENUM_GFX_Line, 
ENUM_GFX_Polygon, 
ENUM_GFX_Polyline, 
ENUM_GFX_CancelMixColor, 
ENUM_GFX_SetMixColor, 
ENUM_GFX_Text, 
ENUM_GFX_BitBlt, 
#endif 

#ifdef OP_CASES 
case ENUM_GFX_C2V_bitBlt: OS_GFX_C2V_bitBlt(window, el->i1, el->i2, el->i3); break; 
case ENUM_GFX_C_DrawCursor: OS_GFX_C_DrawCursor(window, el->i1, el->i2, el->i3, el->i4, el->i5, el->i6); break; 
case ENUM_GFX_P2V_bitBlt: OS_GFX_P2V_bitBlt(window, el->i1, el->i2, el->i3, el->i4, el->i5, el->i6); break; 
case ENUM_GFX_FilledBox: OS_GFX_FilledBox(window, el->i1, el->i2, el->i3, el->i4, el->i5, el->i6); break; 
case ENUM_GFX_Box: OS_GFX_Box(window, el->i1, el->i2, el->i3, el->i4, el->i5, el->i6); break; 
case ENUM_GFX_SetClipRect: OS_GFX_SetClipRect(window, el->i1, el->i2, el->i3, el->i4, el->i5); break; 
case ENUM_GFX_CancelClipRect: OS_GFX_CancelClipRect(window, el->i1); break; 
case ENUM_GFX_Line: PREOS_GFX_Line(window, el->i1, el->i2, el->i3, el->i4, el->i5, el->i6); break; 
case ENUM_GFX_Polygon: OS_GFX_Polygon(window, el->i1, el->i2, el->i3, el->i4, el->i5, el->i6, el->p7, el->i8); break; 
case ENUM_GFX_Polyline: OS_GFX_Polyline(window, el->i1, el->i2, el->i3, el->i4, el->i5, el->i6, el->p7, el->i8); break; 
case ENUM_GFX_CancelMixColor: OS_GFX_CancelMixColor(window); break; 
case ENUM_GFX_SetMixColor: OS_GFX_SetMixColor(window, el->i1, el->i2, el->i3); break; 
case ENUM_GFX_Text: PREOS_GFX_Text(window, el->i1, el->s2, el->i3, el->i4, el->i5, el->i6, el->i7); break; 
case ENUM_GFX_BitBlt: OS_GFX_BitBlt(window, el->i1, el->i2, el->i3, el->i4, el->i5, el->i6); break; 
#endif 

#ifdef OP_FUNCS 
void QUEUE_GFX_C2V_bitBlt(struct Tracker_Windows* window, int from_x1, int from_x2, int to_y){ 
  if(window->must_redraw==true) return; 
  queue_element_t *el = get_next_element(window->op_queue); 
  el->type = ENUM_GFX_C2V_bitBlt ; 
  el->i1 = from_x1 ; 
  el->i2 = from_x2 ; 
  el->i3 = to_y ; 
} 

void QUEUE_GFX_C_DrawCursor(struct Tracker_Windows* window, int x1, int x2, int x3, int x4, int height, int y_pixmap){ 
  if(window->must_redraw==true) return; 
  queue_element_t *el = get_next_element(window->op_queue); 
  el->type = ENUM_GFX_C_DrawCursor ; 
  el->i1 = x1 ; 
  el->i2 = x2 ; 
  el->i3 = x3 ; 
  el->i4 = x4 ; 
  el->i5 = height ; 
  el->i6 = y_pixmap ; 
} 

void QUEUE_GFX_P2V_bitBlt(struct Tracker_Windows* window, int from_x, int from_y, int to_x, int to_y, int width, int height){ 
  if(window->must_redraw==true) return; 
  queue_element_t *el = get_next_element(window->op_queue); 
  el->type = ENUM_GFX_P2V_bitBlt ; 
  el->i1 = from_x ; 
  el->i2 = from_y ; 
  el->i3 = to_x ; 
  el->i4 = to_y ; 
  el->i5 = width ; 
  el->i6 = height ; 
} 

void QUEUE_GFX_FilledBox(struct Tracker_Windows* tvisual, int color, int x, int y, int x2, int y2, int where){ 
  if(tvisual->must_redraw==true) return; 
  queue_element_t *el = get_next_element(tvisual->op_queue); 
  el->type = ENUM_GFX_FilledBox ; 
  el->i1 = color ; 
  el->i2 = x ; 
  el->i3 = y ; 
  el->i4 = x2 ; 
  el->i5 = y2 ; 
  el->i6 = where ; 
} 

void QUEUE_GFX_Box(struct Tracker_Windows* tvisual, int color, int x, int y, int x2, int y2, int where){ 
  if(tvisual->must_redraw==true) return; 
  queue_element_t *el = get_next_element(tvisual->op_queue); 
  el->type = ENUM_GFX_Box ; 
  el->i1 = color ; 
  el->i2 = x ; 
  el->i3 = y ; 
  el->i4 = x2 ; 
  el->i5 = y2 ; 
  el->i6 = where ; 
} 

void QUEUE_GFX_SetClipRect(struct Tracker_Windows* tvisual, int x, int y, int x2, int y2, int where){ 
  if(tvisual->must_redraw==true) return; 
  queue_element_t *el = get_next_element(tvisual->op_queue); 
  el->type = ENUM_GFX_SetClipRect ; 
  el->i1 = x ; 
  el->i2 = y ; 
  el->i3 = x2 ; 
  el->i4 = y2 ; 
  el->i5 = where ; 
} 

void QUEUE_GFX_CancelClipRect(struct Tracker_Windows* tvisual, int where){ 
  if(tvisual->must_redraw==true) return; 
  queue_element_t *el = get_next_element(tvisual->op_queue); 
  el->type = ENUM_GFX_CancelClipRect ; 
  el->i1 = where ; 
} 

void QUEUE_GFX_Line(struct Tracker_Windows* tvisual, int color, int x, int y, int x2, int y2, int where){ 
  if(tvisual->must_redraw==true) return; 
  queue_element_t *el = get_next_element(tvisual->op_queue); 
  el->type = ENUM_GFX_Line ; 
  el->i1 = color ; 
  el->i2 = x ; 
  el->i3 = y ; 
  el->i4 = x2 ; 
  el->i5 = y2 ; 
  el->i6 = where ; 
} 

void QUEUE_GFX_Polygon(struct Tracker_Windows* tvisual, int color, int x1, int y1, int x2, int y2, int num_points, APoint* peaks, int where){ 
  if(tvisual->must_redraw==true) return; 
  queue_element_t *el = get_next_element(tvisual->op_queue); 
  el->type = ENUM_GFX_Polygon ; 
  el->i1 = color ; 
  el->i2 = x1 ; 
  el->i3 = y1 ; 
  el->i4 = x2 ; 
  el->i5 = y2 ; 
  el->i6 = num_points ; 
  el->p7 = peaks ; 
  el->i8 = where ; 
} 

void QUEUE_GFX_Polyline(struct Tracker_Windows* tvisual, int color, int x1, int y1, int x2, int y2, int num_points, APoint* peaks, int where){ 
  if(tvisual->must_redraw==true) return; 
  queue_element_t *el = get_next_element(tvisual->op_queue); 
  el->type = ENUM_GFX_Polyline ; 
  el->i1 = color ; 
  el->i2 = x1 ; 
  el->i3 = y1 ; 
  el->i4 = x2 ; 
  el->i5 = y2 ; 
  el->i6 = num_points ; 
  el->p7 = peaks ; 
  el->i8 = where ; 
} 

void QUEUE_GFX_CancelMixColor(struct Tracker_Windows* tvisual){ 
  if(tvisual->must_redraw==true) return; 
  queue_element_t *el = get_next_element(tvisual->op_queue); 
  el->type = ENUM_GFX_CancelMixColor ; 
} 

void QUEUE_GFX_SetMixColor(struct Tracker_Windows* tvisual, int color1, int color2, int mix_factor){ 
  if(tvisual->must_redraw==true) return; 
  queue_element_t *el = get_next_element(tvisual->op_queue); 
  el->type = ENUM_GFX_SetMixColor ; 
  el->i1 = color1 ; 
  el->i2 = color2 ; 
  el->i3 = mix_factor ; 
} 

void QUEUE_GFX_Text(struct Tracker_Windows* tvisual, int color, const char* text, int x, int y, int width, int flags, int where){ 
  if(tvisual->must_redraw==true) return; 
  queue_element_t *el = get_next_element(tvisual->op_queue); 
  el->type = ENUM_GFX_Text ; 
  el->i1 = color ; 
  memcpy(el->s2, text, R_MIN(strlen(text)+1,62)); 
  el->i3 = x ; 
  el->i4 = y ; 
  el->i5 = width ; 
  el->i6 = flags ; 
  el->i7 = where ; 
} 

void QUEUE_GFX_BitBlt(struct Tracker_Windows* tvisual, int dx, int dy, int x, int y, int x2, int y2){ 
  if(tvisual->must_redraw==true) return; 
  queue_element_t *el = get_next_element(tvisual->op_queue); 
  el->type = ENUM_GFX_BitBlt ; 
  el->i1 = dx ; 
  el->i2 = dy ; 
  el->i3 = x ; 
  el->i4 = y ; 
  el->i5 = x2 ; 
  el->i6 = y2 ; 
} 

#endif 
