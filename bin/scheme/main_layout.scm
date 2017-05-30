
(provide 'main_layout.scm)


(my-require 'notem.scm)



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

(define (lowertab-index-callback index)
  ;;(c-display "\n\n\n                ****lowertab changed to index" index *sequencer-gui-height* "\n\n\n")
  (define handle (<gui> :get-splitter-handle *ysplitter* 1))
  
  (if (string=? (<gui> :tab-name *lowertab-gui* index) *sequencer-gui-tab-name*)
      (let ((seq (<gui> :get-sequencer-gui)))
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
        
        ;;(c-display "minimizing")
        (<gui> :minimize-as-much-as-possible (<gui> :get-instrument-gui))
        (<gui> :minimize-as-much-as-possible *notem-gui*)
        ;;(<gui> :set-size (<gui> :get-instrument-gui) 50 10)
        (<gui> :set-size *lowertab-gui* (<gui> :width *lowertab-gui*) 10))))
  
(define (init-lowertab-gui)
  (<gui> :add-tab *lowertab-gui* *sequencer-gui-tab-name* (<gui> :get-sequencer-gui))
  (if (not (<ra> :position-instrument-widget-in-mixer))
      (<gui> :add-tab *lowertab-gui* *instrument-gui-tab-name* (<gui> :get-instrument-gui)))
  (<gui> :add-tab *lowertab-gui* *edit-gui-tab-name* *notem-gui*)
  (<gui> :set-current-tab *lowertab-gui* 0)
  (<gui> :add-callback *lowertab-gui*
         (lambda (index)
           (lowertab-index-callback index))))


(define (FROM-C-get-lowertab-gui)
  (if (< (<gui> :get-tab-pos *lowertab-gui* (<gui> :get-sequencer-gui))
         0)
      (init-lowertab-gui))
  *lowertab-gui*)


(define (FROM-C-set-lowertab-includes-instrument includeit)
  (define instr (<gui> :get-instrument-gui))
  (define pos (<gui> :get-tab-pos *lowertab-gui* instr))
  (define is-included (>= pos 0))
  ;;(c-display "includeit/pos/is-included" includeit pos is-included)
  
  (cond ((and (not includeit)
              is-included)
         (<gui> :remove-tab *lowertab-gui* pos))
        ((and includeit
              (not is-included))
         (<gui> :add-tab *lowertab-gui* *instrument-gui-tab-name* instr 1))))
      

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
  (if (<ra> :position-instrument-widget-in-mixer)
      (<gui> :is-visible instr)
      (lowertab-gui-is-visible instr)))
                  
(define (FROM-C-show-instrument-gui)
  (show-lowertab-gui (<gui> :get-instrument-gui)))
      
(define (FROM-C-hide-instrument-gui)
  (hide-lowertab-gui (<gui> :get-instrument-gui)))



(define (FROM-C-sequencer-gui-is-visible)
  (lowertab-gui-is-visible (<gui> :get-sequencer-gui)))
    
(define (FROM-C-show-sequencer-gui)
  (show-lowertab-gui (<gui> :get-sequencer-gui)))

(define (FROM-C-hide-sequencer-gui)
  (hide-lowertab-gui (<gui> :get-sequencer-gui)))

    

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
