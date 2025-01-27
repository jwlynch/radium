
(provide 'main_layout.scm)

(my-require 'notem.scm)
(my-require 'sequencer.scm)



;; Main y splitter
;;;;;;;;;;;;;;;;;;;

(define-constant *ysplitter* (if (not (defined? '*ysplitter*))  ;; So that we can reload the file after the program has started.
                                 (<gui> :vertical-splitter)
                                 *ysplitter*))

(define2 *ysplitter-has-initied* boolean? (if (defined? '*ysplitter-has-initied*)
                                              *ysplitter-has-initied*
                                              #f))
                                     
                                          
(define (FROM-C-get-main-y-splitter)
  (when (not *ysplitter-has-initied*)
    (<gui> :add *ysplitter* (<gui> :get-main-x-splitter) 10000000)
    (<gui> :add *ysplitter* (FROM-C-get-lowertab-gui) 0)
    (set! *ysplitter-has-initied* #t))
  *ysplitter*)


#!!
(<gui> :get-splitter-sizes *ysplitter*)
(<gui> :set-splitter-sizes *ysplitter*
       (list (- ((<gui> :get-splitter-sizes *ysplitter*) 0) 100)
             (+ ((<gui> :get-splitter-sizes *ysplitter*) 1) 100)))
       
!!#

;; Lower tab GUI
;;;;;;;;;;;;;;;;;;;;

(define-constant *sequencer-gui-tab-name* "Sequencer")
(define-constant *instrument-gui-tab-name* "Instrument")
(define-constant *edit-gui-tab-name* "Edit")

(define-constant *lowertab-gui* (if (defined? '*lowertab-gui*) ;; So that we can reload the file after the program has started.
                                    *lowertab-gui*
                                    (my-tabs #f)))

(define2 *sequencer-gui-height* (curry-or not integer?) #f)
(define2 *lowertab-gui-height* (curry-or not integer?) #f)

(define2 *curr-lowertab-is-sequencer* boolean? #t)

(define (minimize-lowertab)
  ;;(c-display "minimizing")
  (<gui> :minimize-as-much-as-possible (<gui> :get-instrument-gui))
  (<gui> :minimize-as-much-as-possible *notem-gui*)
  ;;(<gui> :set-size (<gui> :get-instrument-gui) 50 10)
  (<gui> :set-size *lowertab-gui* (<gui> :width *lowertab-gui*) 10))
  
(define (lowertab-index-callback index)
  ;;(c-display "\n\n\n                ****lowertab changed to index" index *sequencer-gui-height* "\n\n\n")
  (define handle (<gui> :get-splitter-handle *ysplitter* 1))
  
  (if (string=? (<gui> :tab-name *lowertab-gui* index) *sequencer-gui-tab-name*)
      (begin
        (<gui> :set-enabled handle #t)
        (set! *curr-lowertab-is-sequencer* #t)
        (if *sequencer-gui-height*
            (<gui> :set-size *lowertab-gui* (<gui> :width *lowertab-gui*) *sequencer-gui-height*)))
      (begin
        (when *curr-lowertab-is-sequencer*
          (if (and (<gui> :is-visible *lowertab-gui*)
                   (> (<gui> :height *lowertab-gui*) 0))
              (set! *sequencer-gui-height* (<gui> :height *lowertab-gui*)))
          (set! *curr-lowertab-is-sequencer* #f))
        
        (<gui> :set-enabled handle #f)

        (minimize-lowertab))))


(define (init-lowertab-gui)
  
  (define instrument-gui (<gui> :get-instrument-gui))
  (define sequencer-gui (<gui> :get-sequencer-gui))
  (define sequencer-frame-gui (<gui> :get-sequencer-frame-gui))

  ;; Try to make tabs have same size
  (<gui> :minimize-as-much-as-possible sequencer-gui)
  (<gui> :minimize-as-much-as-possible instrument-gui)
  (<gui> :minimize-as-much-as-possible *notem-gui*)
  
  (define width (max (<gui> :width sequencer-frame-gui)
                     (<gui> :width instrument-gui)
                     (<gui> :width *notem-gui*)))
  (define height (max (<gui> :height sequencer-frame-gui)
                      (<gui> :height instrument-gui)
                      (<gui> :height *notem-gui*)))

  (<gui> :set-size instrument-gui width height)
  (<gui> :set-size sequencer-frame-gui width height)
  (<gui> :set-size *notem-gui* width height)
  
  (<gui> :add-tab *lowertab-gui* *sequencer-gui-tab-name* sequencer-frame-gui)
  
  (if (not (<ra> :instrument-widget-is-in-mixer))
      (<gui> :add-tab *lowertab-gui* *instrument-gui-tab-name* instrument-gui))
  
  (<gui> :add-tab *lowertab-gui* *edit-gui-tab-name* *notem-gui*)

  ;;(<gui> :set-current-tab *lowertab-gui* 0)
  
  (<gui> :add-callback *lowertab-gui*
         (lambda (index)
           (lowertab-index-callback index)))

  ;; Make sure tab bar is drawn in correct size from the beginning.
  (set-fixed-height (<gui> :get-tab-bar *lowertab-gui*) height) ;; Hack. Calling (<gui> :set-height *lowertab-gui*) doesn't work immediately. TODO: Investigate why.
  
  )


(define (FROM-C-get-lowertab-gui)
  (if (or (not (<gui> :get-sequencer-frame-gui))
          (< (<gui> :get-tab-pos *lowertab-gui* (<gui> :get-sequencer-frame-gui))
             0))
      (init-lowertab-gui))
  *lowertab-gui*)


(define (FROM-C-set-lowertab-includes-instrument includeit)
  (define instr (<gui> :get-instrument-gui))
  (define pos (<gui> :get-tab-pos *lowertab-gui* instr))
  (define is-included (>= pos 0))
  ;;(c-display "includeit/pos/is-included" includeit pos is-included)
  
  (cond ((and (not includeit)
              is-included)
         (if (= 1 (<gui> :current-tab *lowertab-gui*))
             (<gui> :set-current-tab *lowertab-gui* 0))
         (<gui> :remove-tab *lowertab-gui* pos))
        ((and includeit
              (not is-included))
         (<gui> :add-tab *lowertab-gui* *instrument-gui-tab-name* instr 1)
         (<gui> :set-current-tab *lowertab-gui* 1))))
      

(define (show-lowertab-gui gui)
  (define pos (<gui> :get-tab-pos *lowertab-gui* gui))
  (when (>= pos 0)
    (if (not (<gui> :is-visible *lowertab-gui*))
        (<gui> :show *lowertab-gui*))
    (<gui> :set-current-tab *lowertab-gui* pos)))

(define (hide-lowertab-gui gui)
  (define pos (<gui> :get-tab-pos *lowertab-gui* gui))
  (when (>= pos 0)
    (when (<gui> :is-visible *lowertab-gui*)
      (if (= (<gui> :current-tab *lowertab-gui*) pos)
          (<gui> :hide *lowertab-gui*)))))


(define (lowertab-gui-is-visible gui)
  (and (<gui> :is-visible *lowertab-gui*)
       (= (<gui> :get-tab-pos *lowertab-gui* gui)
          (<gui> :current-tab *lowertab-gui*))))



(define (FROM-C-instrument-gui-is-visible)
  (define instr (<gui> :get-instrument-gui))
  (if (<ra> :instrument-widget-is-in-mixer)
      (<gui> :is-visible instr)
      (lowertab-gui-is-visible instr)))
                  
(define (FROM-C-show-instrument-gui)
  (show-lowertab-gui (<gui> :get-instrument-gui)))
      
(define (FROM-C-hide-instrument-gui)
  (hide-lowertab-gui (<gui> :get-instrument-gui)))


;; Remove from main tabs, and open in new window
(define (move-sequencer-to-window)
  (assert (not *sequencer-window-gui-active*))
  (set! *sequencer-window-gui-active* #t)
  
  (let* ((sequencer-gui (<gui> :get-sequencer-frame-gui))
         (has-window *sequencer-window-gui*)
         (window (if has-window
                     *sequencer-window-gui*
                     (let ((window (<gui> :vertical-layout)))
                       (<gui> :set-size
                              window
                              (<gui> :width (<gui> :get-parent-window sequencer-gui))
                              (+ (floor (* (get-fontheight) 1.5))
                                 (<gui> :height sequencer-gui)))
                       (<gui> :set-takes-keyboard-focus window #f)
                       window))))
    
    (<gui> :remove-parent sequencer-gui)
    
    (define bottom-bar (if has-window
                           (let ((bottom-bar (<gui> :child window "bottom-bar")))
                             (<gui> :remove-parent bottom-bar)
                             bottom-bar)
                           (let ((bottom-bar (<gui> :bottom-bar #f #f)))
                             (<gui> :set-name bottom-bar "bottom-bar")
                             bottom-bar)))
    
    (<gui> :add window sequencer-gui)
    (<gui> :add window bottom-bar)
    
    (<gui> :set-as-window window (if (<ra> :sequencer-window-is-child-of-main-window)
                                     -1
                                     -3
                                     ))
    (<gui> :show sequencer-gui)
    (<gui> :show window)
    
    (when (not has-window)

      (<gui> :add-close-callback window
             (lambda (radium-runs-custom-exec)
               ;;(move-sequencer-to-main-tabs)
               (<gui> :hide window)
               #f))
      
      (set! *sequencer-window-gui* window))
    
    ))


;; Remove from sequencer window, add to main tabs, hide sequencer window gui.
(define (move-sequencer-to-main-tabs)
  (assert *sequencer-window-gui-active*)    
  (set! *sequencer-window-gui-active* #f)
    
  (let ((sequencer-gui (<gui> :get-sequencer-frame-gui))
        (window *sequencer-window-gui*))
    (<gui> :remove-parent sequencer-gui)
    (<gui> :add-tab *lowertab-gui* *sequencer-gui-tab-name* sequencer-gui 0)
    (<gui> :set-current-tab *lowertab-gui* 0)
    (<gui> :hide window)))


#!!
(<ra> :show-upper-part-of-main-window)
(<ra> :hide-upper-part-of-main-window)
!!#


(define (FROM-C-sequencer-set-gui-in-window! doit)
  (cond ((and doit
              (not *sequencer-window-gui-active*))
         (move-sequencer-to-window))
        ((and (not doit)
              *sequencer-window-gui-active*)
         (move-sequencer-to-main-tabs))))
      
(define (FROM-C-sequencer-gui-in-window)
  *sequencer-window-gui-active*)

(define (FROM-C-sequencer-gui-is-visible)
  (let ((ret (or (and *sequencer-window-gui-active*
                      (<gui> :is-visible *sequencer-window-gui*))
                 (lowertab-gui-is-visible (<gui> :get-sequencer-frame-gui)))))
    (c-display "sequencer visible:" ret)
    ret))

(define (FROM-C-show-sequencer-gui)
  (<ra> :set-sequencer-keyboard-focus)
  (if *sequencer-window-gui-active*
      (<gui> :show *sequencer-window-gui*)
      (show-lowertab-gui (<gui> :get-sequencer-frame-gui))))

(define (FROM-C-hide-sequencer-gui)
  (if *sequencer-window-gui-active*
      (<gui> :hide *sequencer-window-gui*)
      (hide-lowertab-gui (<gui> :get-sequencer-frame-gui))))


#!!
(FROM-C-show-instrument-gui)
(FROM-C-hide-instrument-gui)
(begin *sequencer-window-gui-active*)
(<ra> :sequencer-is-visible)
(<ra> :hide-sequencer)
!!#


(define (FROM-C-edit-gui-is-visible)
  (lowertab-gui-is-visible *notem-gui*))
    
(define (FROM-C-show-edit-gui)
  (<ra> :set-editor-keyboard-focus)
  (show-lowertab-gui *notem-gui*))

(define (FROM-C-hide-edit-gui)
  (hide-lowertab-gui *notem-gui*))

(define (FROM-C-get-edit-gui)
  *notem-gui*)


(define (FROM-C-show-edit/quantitize-tab)
  (define pos (<gui> :get-tab-pos *notem-gui* *quantitize-tab*))
  (<gui> :set-current-tab *notem-gui* pos)
  (show-lowertab-gui *notem-gui*))

(define (FROM-C-show-edit/transpose-tab)
  (define pos (<gui> :get-tab-pos *notem-gui* *transpose-tab*))
  (<gui> :set-current-tab *notem-gui* pos)
  (show-lowertab-gui *notem-gui*))

(define (FROM-C-show-edit/randomize-tab)
  (define pos (<gui> :get-tab-pos *notem-gui* *randomize/skew-notem-tab*))
  (<gui> :set-current-tab *notem-gui* pos)
  (show-lowertab-gui *notem-gui*))

(define (FROM-C-show-edit/various-tab)
  (define pos (<gui> :get-tab-pos *notem-gui* *various-tab*))
  (<gui> :set-current-tab *notem-gui* pos)
  (show-lowertab-gui *notem-gui*))


#!!
(FROM-C-sequencer-gui-is-visible)

(<gui> :set-background-color (<gui> :get-sequencer-gui) "color1")
(<gui> :set-background-color (<gui> :get-sequencer-gui) "black")

(<gui> :set-background-color *lowertab-gui* "high_background_color")
(<gui> :set-background-color *lowertab-gui* "color1")
(<gui> :set-background-color *lowertab-gui* "black")

(<gui> :hide *lowertab-gui*)
(<gui> :show *lowertab-gui*)
(update-lower-tab)
(<gui> :set-size-policy *lowertab-gui* #t #t)

(<gui> :hide (<gui> :get-instrument-gui))
(<gui> :show (<gui> :get-instrument-gui))

(<gui> :set-size *lowertab-gui* 100 100)

(<gui> :minimize-as-much-as-possible *lowertab-gui*)
(<gui> :minimize-as-much-as-possible (<gui> :get-instrument-gui))

(let ((inst (<gui> :get-instrument-gui)))
  (<gui> :set-min-height *lowertab-gui* (<gui> :height inst)))


(<gui> :set-min-height *lowertab-gui* 5)


(begin *lowertab-tabs*)

(<gui> :set-style-sheet-recursively *lowertab-gui*
       (<-> "QScrollArea"
            "{"
            "  background-color: transparent;"
            "}"
            ;;"QTabBar { background: green; }"
            "QScrollArea > QWidget > QWidget { background: transparent; }"
            "QScrollArea > QWidget > QScrollBar { background: rgba(ff, ff, ff, 50); }"
            ))


!!#
