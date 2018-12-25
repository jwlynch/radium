;;
;; A very lightweight and super simple widget manager.
;;
;; This code is originally taken from hurtigmixer (https://github.com/kmatheussen/hurtigmixer/blob/master/src/area.scm),
;; but slightly simplified and modified. Blitting has, for instance, been removed since we don't
;; need it. It might be put back later if needed.

;; All x and y values are according to the underlying Qt widget, and not according to the area.
;; (makes everything much simpler, and straight forward)

;; All x and y values can be floating points (QWidget's x and y values must be integers).

;; Subclasses of def-area must define x1, y1, x2, y2, and gui. All of these would usually be provided as class parameters.

;; Subclasses can implement the following virtual methods: ismoved, key-pressed, paint, post-paint, etc.
;; Areas only holding sub areas will often not implement any of these, and for those areas, we can simply
;; use the ready-made (and ultimately simple) <area> class instead (see below).

;; How to implement a custom method in an area subclass:
;;  (def-area-subclass (<area-with-custom-method> :gui :x1 :y1 :x2 :y2)  
;;    :custom-method ()
;;    (c-display "this text was printed from a custom method!"))
;; i.e. just like you normally would; methods go at the end of the class.
;; EDIT: That doesn't work anymore. The body of the subclasses was later put into their own scope to avoid accidentally
;; overriding symbols in the superclass. Instead you need to use :add-method! manually, after an instance has been created.
;; That's very inconvenient though, so maybe this should be improved.


(provide 'area.scm)

(my-require 'gui.scm)


#!!
(gc #t)
(set! (*s7* 'gc-stats) #f)
!!#

(define (myfloor a)
  ;;a
  (floor a)
  )


(define-expansion (define-override def . body)
  (let* ((funcname (car def))
         (org (<_> "super:" funcname)))
    `(let ((,org ,funcname))
       (set! ,funcname (lambda ,(cdr def)
                         ,@body)))))


(define-expansion (def-area-subclass def . body)

  (define body-methods '())
  (let ((temp (split-list body keyword?)))
    (set! body (car temp))
    (set! body-methods (cadr temp)))
                     
  `(define-class ,def

     ;; To avoid overlapping paint updates, we convert all coordinates to integers.
     ;; (Qt uses integers in the widget update system, so if we use floats here, widgets on all sides of the area will be repainted unnecessarily.)
     (set! x1 (myfloor x1))
     (set! y1 (myfloor y1))
     (set! x2 (myfloor x2))
     (set! y2 (myfloor y2))
     
     (define width (- x2 x1))
     (define height (- y2 y1))

     (define (paint?)
       #t)

     (define is-alive #t)

     ;; Position
     
     (define (get-position callback)
       (callback x1 y1 x2 y2 width height))
     
     (define i-x1 #f)
     (define i-y1 #f)
     (define i-x2 #f)
     (define i-y2 #f)
     
     ;; optimization to avoid inside? to call parent-area::inside?
     (define (set-i-variables!)
       (when (not i-x1)
         (if parent-area
             (parent-area :get-i-position
                          (lambda (px1 py1 px2 py2)
                            (set! i-x1 (max x1 px1))
                            (set! i-y1 (max y1 py1))
                            (set! i-x2 (min x2 px2))
                            (set! i-y2 (min y2 py2))))
             (begin
               (set! i-x1 x1)
               (set! i-y1 y1)
               (set! i-x2 x2)
               (set! i-y2 y2)))))
       
     (define (get-i-position callback)
       (set-i-variables!)
       (callback i-x1 i-y1 i-x2 i-y2))
     
     ;; We return false if x* and y* aren't inside the parent either.
     (define (inside? x* y*)
       (set-i-variables!)
       (and (>= x* i-x1)
            (< x* i-x2)
            (>= y* i-y1)
            (< y* i-y2)))

     (define (overlaps? x1* y1* x2* y2*)
       (and (> x2* x1)
	    (< x1* x2)
	    (> y2* y1)
	    (< y1* y2)))

     (define ismoved #f)

     (define (move-internal! dx dy)
       (set! i-x1 #f)
       (inc! x1 dx)
       (inc! y1 dy)
       (inc! x2 dx)
       (inc! y2 dy)
       (for-each (lambda (sub-area)
		   (sub-area :move-internal! dx dy))
		 sub-areas)
       (if ismoved
	   (ismoved)))
     
     (define (move! dx dy)
       (let ((old-x1 x1)
             (old-x2 x2)
             (old-y1 y1)
             (old-y2 y2))
         ;;(update-me!)
         (move-internal! dx dy)
         (update (min old-x1 x1)
                 (min old-y1 y1)
                 (max old-x2 x2)
                 (max old-y2 y2))))

     (define (update x1* y1* x2* y2*)
       (let ((x1 (max x1 x1*))
             (y1 (max y1 y1*))
             (x2 (min x2 x2*))
             (y2 (min y2 y2*)))
         ;;(c-display "     UPDATE" x1 y1 x2 y2)
         (if (and (> x2 x1)
                  (> y2 y1))
             (<gui> :update gui x1 y1 x2 y2)
             (c-display "Warning, illegal parameters for update: " x1 y1 x2 y2))))

     (define (update-me!)
       ;;(c-display "     UPDATE-ME!" x1 y1 x2 y2)
       (<gui> :update gui x1 y1 x2 y2))
      
     (define (set-position! x* y*)
       (let ((dx (- x* x1))
	     (dy (- y* y1)))
	 (move! dx dy)))     

     (define (set-position2! x1* y1* x2* y2*)
       (define new-width (- x2* x1*))
       (define new-height (- y2* y1*))
       (set! x2 (+ x1 new-width))
       (set! y2 (+ y1 new-height))
       (set-position! x1* y1*))

     (define effect-monitors '())

     (define (add-area-effect-monitor! instrument-id effect-name monitor-stored monitor-automation callback)
       (push-back! effect-monitors (<ra> :add-effect-monitor effect-name instrument-id monitor-stored monitor-automation
                                         (lambda (radium-normalized automation)
                                           (if (<gui> :is-open gui)
                                               (callback radium-normalized automation)
                                               (c-display "---" ',(car def) "add-area-effect-monitor!: Warning! gui" gui " has been closed"))))))

     (define (remove-sub-areas!)
       (for-each (lambda (effect-monitor)
                   (<ra> :remove-effect-monitor effect-monitor #f))
                 effect-monitors)
       (set! effect-monitors '())

       (for-each (lambda (sub-area)
                   (sub-area :i-am-removed-internal!))
                 sub-areas)
       (set! sub-areas '())
       (set! top-area #f))

     (define i-am-removed! #f)

     (define (i-am-removed-internal!)
       (if i-am-removed!
           (i-am-removed!))
       (remove-sub-areas!)
       (set! is-alive #f))

     (define (reset! x1* y1* x2* y2*)
       (set-position2! x1* y1* x2* y2*)
       (remove-sub-areas!)
       (set! parent-area #f)
       (update-me!))

     (define parent-area #f)

     ;; Sub areas
     (define sub-areas '())
     (define top-area #f)

     (define (add-sub-area-plain! sub-area)
       (push-back! sub-areas sub-area)
       (set! top-area sub-area)
       (sub-area :set-parent-area! this)
       (sub-area :update-me!))

     (define (add-sub-area! sub-area x y)
       ;;(c-display " THIS10:" this x y ". sub-area:" sub-area)
       (sub-area :set-position! x y)

       (add-sub-area-plain! sub-area)
       ;;(c-display " THIS:" this)

       (sub-area :get-position
                 (lambda (x* y* x2* y2* with* height*)
                   (if (inside? x* y*)
                       (update x* y* x2* y2*))))
       ;;(c-display "sub-area added to" ',(car def) ". New sub-area length:" (length sub-areas))
       )

     (define (add-sub-area-above! sub-area-below sub-area)
       (sub-area-below :get-position
                       (lambda (x1 y1 x2 y2 width height)
                         (add-sub-area! sub-area x2 y1))))
     
     (define (add-sub-area-below! above-sub-area sub-area)
       (sub-area-above :get-position
                       (lambda (x1 y1 x2 y2 width height)
                         (add-sub-area! sub-area x1 y2))))
     
     (define (remove-sub-area! sub-area)
       (sub-area :i-am-removed-internal!)
       (set! sub-areas (delete sub-area sub-areas eq?))
       (set! top-area
	     (if (null? sub-areas)
		 #f
		 (last sub-areas)))
       )

     (define (lift-sub-area! sub-area)
       (when (not (eq? sub-area top-area))
	 (set! sub-areas (append (delete sub-area sub-areas eq?)
                                 (list sub-area)))
	 (set! top-area sub-area)
         (sub-area :get-position
                   (lambda (x* y* x2* y2* with* height*)
                     (update x* y* x2* y2*)))))

     
     (define (lift-me!)
       (if parent-area
           (parent-area :lift-sub-area! this)))
     
     ;; Keyboard listener
     (define key-pressed #f)
     (define (key-pressed-internal key-event)
       (call-with-exit
	(lambda (return)
	  (if key-pressed
	      (let ((ret (key-pressed key-event)))
		(if ret
		    (return #t)))
	      (for-each (lambda (sub-area)
			  (if (sub-area :key-pressed-internal key-event)
			      (return #t)))
			sub-areas))
	  #f)))

     (define (key-released-internal key-event)
       ;;(c-display "released something")
       #t
       )

     
     ;; Mouse cycles
     ;;;;;;;;;;;;;;;;;;;;;;;;
     
     (define curr-nonpress-mouse-cycle #f)
     (define curr-mouse-cycle #f)


     (define mouse-cycles '())
     (define nonpress-mouse-cycles '())
       
     (delafina (add-nonpress-mouse-cycle! :enter-func (lambda x #t)
                                          :move-func (lambda x #f)
                                          :leave-func (lambda x #f)) ;; Leave area, or button was pressed.
       (push-back! nonpress-mouse-cycles
                   (make-mouse-cycle enter-func
                                     move-func
                                     leave-func
                                     inside?))
       )

     (define (start-nonpress-mouse-cycle! new-cycle)
       (assert (not curr-nonpress-mouse-cycle))
       (set! curr-nonpress-mouse-cycle new-cycle))
     
     (define (end-nonpress-mouse-cycle!)
       (assert curr-nonpress-mouse-cycle)
       (let ((nonpress-mouse-cycle curr-nonpress-mouse-cycle))
         (set! curr-nonpress-mouse-cycle #f)
         ;;(c-display "end-nonpress-mouse-cycle called" (nonpress-mouse-cycle :release-func))
         ((nonpress-mouse-cycle :release-func))))
  
     (delafina (add-mouse-cycle! :press-func (lambda x #t)
                                 :drag-func (lambda x #f)
                                 :release-func (lambda x #f))
       (push-back! mouse-cycles
                   (make-mouse-cycle press-func drag-func release-func))
       )

     (delafina (add-delta-mouse-cycle! :press-func (lambda x #t)
                                       :drag-func (lambda x #f)
                                       :release-func (lambda x #f))
       (define prev-x #f)
       (define prev-y #f)
       (define inc-x 0)
       (define inc-y 0)
       (define (call-drag-func button x* y*)
         (define dx (cond ((only-y-direction)
                           0)
                          ((<ra> :control-pressed)
                           (/ (- x* prev-x)
                              10))
                          (else
                           (- x* prev-x))))
         (define dy (cond ((only-x-direction)
                           0)
                          ((<ra> :control-pressed)
                           (/ (- y* prev-y)
                              10))
                          (else
                           (- y* prev-y))))
         (when (or (not (= 0 dx))
                   (not (= 0 dy)))
           (inc! inc-x dx)
           (inc! inc-y dy)
           ;;(c-display "dx:" dx ". inc-x:" inc-x)
           (drag-func button x* y* inc-x inc-y))
         (set! prev-x x*)
         (set! prev-y y*))

       (push-back! mouse-cycles
                   (make-mouse-cycle (lambda (button x* y*)
                                       (set! prev-x x*)
                                       (set! prev-y y*)
                                       (set! inc-x 0)
                                       (set! inc-y 0)
                                       (press-func button x* y*))
                                     call-drag-func
                                     (lambda (button x* y*)
                                       (call-drag-func button x* y*)
                                       (release-func button x* y*)))))
                         

     (define (get-nonpress-mouse-cycle x* y*)
       (and (paint?)
            (inside? x* y*)
	    (or (call-with-exit (lambda (return)
                                  (for-each (lambda (sub-area)
                                              (and-let* ((res (sub-area :get-nonpress-mouse-cycle x* y*)))
                                                        (return res)))
                                            (reverse sub-areas))
                                  #f))
		(call-with-exit (lambda (return)                                  
                                  (for-each (lambda (mouse-cycle)
                                              (and-let* ((res (mouse-cycle :press-func x* y*)))
                                                        (return mouse-cycle)))
                                            nonpress-mouse-cycles)
                                  #f)))))
       
     

     (define (get-mouse-cycle button x* y*)
       (and (paint?)
            (inside? x* y*)
	    (or (call-with-exit (lambda (return)
                                  (for-each (lambda (sub-area)
                                              (and-let* ((res (sub-area :get-mouse-cycle button x* y*)))
                                                        (return res)))
                                            (reverse sub-areas))
                                  #f))
		(call-with-exit (lambda (return)
                                  (for-each (lambda (mouse-cycle)
                                              (and-let* ((res (mouse-cycle :press-func button x* y*)))
                                                        (return mouse-cycle)))
                                            mouse-cycles)
                                  #f)))))

     (define (mouse-press-internal button x* y*)
       ;;(c-display "_____________________________________mouse-press" curr-mouse-cycle)
       (if curr-nonpress-mouse-cycle
           (end-nonpress-mouse-cycle!))
       (if (not (<ra> :release-mode))
           (assert (not curr-mouse-cycle))) ;; Unfortunately, we can't trust Qt to send release events. (fixed now)
       (set! curr-mouse-cycle (get-mouse-cycle button x* y*))
       ;;(c-display "====-------Setting curr-mouse-cycle to:" curr-mouse-cycle)
       ;;(<ra> :show-warning "gakk")
       curr-mouse-cycle)
     
     (define (mouse-move-internal button x* y*)
       (cond (curr-mouse-cycle
              (curr-mouse-cycle :drag-func button x* y*))
             (curr-nonpress-mouse-cycle
              ;;(c-display "inside?" class-name y1 y2 (curr-nonpress-mouse-cycle :inside? x* y*))
              (if (curr-nonpress-mouse-cycle :inside? x* y*)
                  (curr-nonpress-mouse-cycle :drag-func x* y*) ;; still inside
                  (end-nonpress-mouse-cycle!))) ;; not inside any more
             (else
              (start-nonpress-mouse-cycle! (get-nonpress-mouse-cycle x* y*)))))
     (define (mouse-release-internal button x* y*)
       ;;(c-display "mouse-release enter" curr-mouse-cycle)
       (let ((mouse-cycle curr-mouse-cycle))
         (set! curr-mouse-cycle #f)
         ;;(c-display "===------ Unsetting curr-mouse-cycle");
         (if mouse-cycle
             (mouse-cycle :release-func button x* y*)))
       ;;(c-display "mouse-release leave" curr-mouse-cycle)
       )

     (define (mouse-callback-internal button state x y)

       ;;(c-display "   mouse-callback-internal" "has:" (if curr-mouse-cycle #t #f) ". button/state:" button state
       ;;           (if (= state *is-releasing*) "releasing" (if (= state *is-leaving*) "leaving" (if (= state *is-pressing*) "pressing" "unknown"))))
       
       ;; make sure release is always called when releasing, no matter other states.
       (when (or (= state *is-releasing*)
                 (= state *is-leaving*))
         ;;(c-display "     MOUSE-CALLBACK-INTERNAL called for" class-name)
         (mouse-release-internal button x y))
       
       (cond (*current-mouse-cycle*
              #f) ;; i.e. mouse.scm is handling mouse now.
             ((= state *is-leaving*)
              (if curr-nonpress-mouse-cycle
                  (end-nonpress-mouse-cycle!)
                  #f))
             ((= state *is-moving*)
              (mouse-move-internal button x y))
             ((not (inside? x y))
              #f)
             ((= state *is-pressing*)
              (mouse-press-internal button x y))
             ((= state *is-releasing*)
              #f)
             (else
              (assert (= state *is-entering*))
              #f)))

     (define (has-mouse)
       ;;(c-display "has-mouse:" class-name curr-mouse-cycle curr-nonpress-mouse-cycle)
       (get-bool (or curr-mouse-cycle curr-nonpress-mouse-cycle)))
     
     ;; Status bar
     ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
     
     (define statusbar-text-id -1)
     
     (define (set-statusbar-text! text)
       (set! statusbar-text-id (<ra> :set-statusbar-text text)))

     (define (remove-statusbar-text)
       (<gui> :tool-tip "")
       (<ra> :remove-statusbar-text statusbar-text-id))
     
     (define (add-statusbar-text-handler string-or-func)
       (add-nonpress-mouse-cycle!
        :enter-func (lambda (x* y)
                      (define string-or-pair (if (procedure? string-or-func)
                                                 (string-or-func)
                                                 string-or-func))
                      (define text (if (pair? string-or-pair)
                                       (cadr string-or-pair)
                                       string-or-pair))
                      (define also-show-tooltip (if (pair? string-or-pair)
                                                    (car string-or-pair)
                                                    #f))
                      
                      (if also-show-tooltip
                          (<gui> :tool-tip text))
                      (set-statusbar-text! text)
                      #t)
        :leave-func remove-statusbar-text))
       
     ;; Painting
     ;;;;;;;;;;;;;;;
       
     (define (paint)  ;; Called before painting the current area's sub-areas
       #f)
     (define (post-paint) ;; Called after painting the current area's sub-areas
       #f)
     
     (define (paint-internal px1 py1 px2 py2) ;; px1, py1, etc. is the clip area of the parent area.

       ;;(c-display "\n\npaint-internal called" ',(car def) "(" x1 y1 x2 y2 "). p: (" px1 py1 px2 py2 ")")
                  
       
       '(c-display (paint?)
                   (<gui> :area-needs-painting gui x1 y1 x2 y2));;overlaps? x1* y1* x2* y2*))

       (when (and (paint?)
                  (<gui> :area-needs-painting gui x1 y1 x2 y2));;overlaps? x1* y1* x2* y2*))

         ;;(c-display "paint-internal hepp" ',(car def) paint "sub-areas" sub-areas)
         (let ((cx1 (max x1 px1))
               (cy1 (max y1 py1))
               (cx2 (min x2 px2))
               (cy2 (min y2 py2)))

           (when (and (> cx2 cx1)
                      (> cy2 cy1))

             (<gui> :set-clip-rect gui cx1 cy1 cx2 cy2)
             (paint)
             
             (for-each (lambda (sub-area)
                         (sub-area :paint-internal cx1 cy1 cx2 cy2))
                       sub-areas)
             
             (<gui> :set-clip-rect gui cx1 cy1 cx2 cy2)
             (post-paint)))
           
         (if (not parent-area)
             (<gui> :cancel-clip-rect gui))))
     
     (define class-name ',(car def))

     (let () ;; Put body into new scope to avoid accidentally overriding an internal method. (use define-override instead of define to purposefully override)
       #t ;; Added to silence "let has no body" error messages.
       ,@body)

     ,@body-methods

     :get-width () width
     :get-height () height
     :get-position x (apply get-position x)
     :get-i-position x (apply get-i-position x)
     :inside? x (apply inside? x)
     :update-me! x (apply update-me! x)
     :set-position! x (apply set-position! x)
     :set-position2! x (apply set-position2! x)
     :move! x (apply move! x)
     :move-internal! x (apply move-internal! x)
     :set-parent-area! (new-parent-area) (begin
                                           (assert new-parent-area)
                                           (set! parent-area new-parent-area))
     :add-sub-area-plain! (sub-area) (add-sub-area-plain! sub-area)
     :add-sub-area! x (apply add-sub-area! x)
     :add-sub-area-above! x (apply add-sub-area-above! x)
     :add-sub-area-below! x (apply add-sub-area-below! x)
     :remove-sub-area! x (apply remove-sub! x)
     :remove-sub-areas! x (apply remove-sub-areas! x)
     :lift-sub-area! x (apply lift-sub-area! x)
     :lift-me! x (apply lift-me! x)
     :key-pressed-internal! x (apply key-pressed-internal! x)
     :key-released-internal! x (apply key-released-internal! x)
     :add-mouse-cycle! x (apply add-mouse-cycle! x)
     :get-mouse-cycle x (apply get-mouse-cycle x)
     :add-nonpress-mouse-cycle! x (apply add-nonpress-mouse-cycle! x)
     :get-nonpress-mouse-cycle x (apply get-nonpress-mouse-cycle x)
     :overlaps? x (apply overlaps? x)
     :paint-internal x (apply paint-internal x)
     :mouse-callback-internal x (apply mouse-callback-internal x)
     :has-mouse () (has-mouse)
     :reset! x (apply reset! x)
     :i-am-removed-internal! x (apply i-am-removed-internal! x)
     :add-statusbar-text-handler x (apply add-statusbar-text-handler x)
     ))
 


(def-area-subclass (<area> :gui :x1 :y1 :x2 :y2)  
  )


(delafina (make-qtarea :width 100 :height 100)
  (define gui (<gui> :widget width height))  
  (define x1 0)
  (define y1 0)
  (define x2 width)
  (define y2 height)
  (def-area-subclass (<qtarea>)
    (<gui> :add-paint-callback gui
           (lambda (width height)
             (paint-internal 0 0 width height)))
    
    (<gui> :add-mouse-callback gui
           (lambda (button state x y)
             (mouse-callback-internal button state x y)
             ;;(c-display "has-mouse:" (and (defined? 'has-mouse) (has-mouse)))
             ;;50))
             (has-mouse)))
    :get-gui () gui)

  (define area (<new> :qtarea))

  (area :add-method! :get-gui (lambda ()
                                gui))

  area)



(define *use-testgui* #f)


#!!
(def-area-subclass (<testarea> :gui :x1 :y1 :x2 :y2)  
  (define X 0)
  (define Y 0)

  (define-override (paint)
    ;;(c-display "x1:" gui x1 y1 x2 y2 (<ra> :generate-new-color))
    (<gui> :filled-box gui (<ra> :generate-new-color 1) x1 y1 x2 y2)
    (<gui> :draw-text gui "green" "hello" X Y x2 y2)
    (<gui> :draw-line gui "white" X Y x2 y2 2.3))
  
  (add-mouse-cycle! (lambda (button x* y*)
                      (set! X x*)
                      (set! Y y*)
                      (update X Y x2 y2)
                      (c-display "press button/x/y" x* y*))
                    (lambda (button x* y*)
                      (set! X x*)
                      (set! Y y*)
                      (update X Y x2 y2)
                      (c-display "move button/x/y" x* y*))
                    (lambda (button x* y*)
                      (set! X x*)
                      (set! Y y*)
                      (update X Y x2 y2)
                      (c-display "release button/x/y" x* y*)))
  )


(pretty-print (macroexpand (def-area-subclass (<area-with-custom-method> :gui :x1 :y1 :x2 :y2)
                             (this :add-method 'custom-method (lambda ()
                                                                (c-display "this text was printed from a custom method!"))))
                           )
              )



(define testarea2 (<new> :area-with-custom-method *testarea* 0 0 100 100))))

(pretty-print (macroexpand (<new> :area-with-custom-method *testarea* 0 0 100 100)))
(testarea2 :add-method! :ai (lambda () (c-display "hello")))

(testarea2 :custom-method)
(testarea2 :ai)

(if (and (defined? '*testgui*)
         *testgui*
         (<gui> :is-open *testgui*))
    (<gui> :close *testgui*))

(define *testgui* (and *use-testgui*
                       (<gui> :widget 500 500)))


;; Save some cycles by not painting background color if only vertical audio meters are updated (meters are repainted continously)
(when *use-testgui*
  (<gui> :dont-autofill-background *testgui*)
  (<gui> :set-background-color *testgui* (<gui> :get-background-color *testgui*)))


!!#

#||
(define testarea (<new> :testarea *testgui* 100 200 1100 1200))

(testarea :get-position c-display)

(<gui> :show *testgui*)

(<gui> :add-paint-callback *testgui*
       (lambda (width height)
         (testarea :paint-internal 0 0 width height)))

(<gui> :add-mouse-callback *testgui*
       (lambda (button state x y)
         (c-display "asd" x y)
         (testarea :mouse-callback-internal button state x y)
         (if (testarea :has-mouse)
             #t
             #f)))
||#

                     
(def-area-subclass (<text-area> :gui :x1 :y1 :x2 :y2
                                :text ;; can also be function
                                :background-color #f ;; If #f, background will not be painted. can also be function
                                :text-color *text-color* ;; can also be function
                                :wrap-lines #f
                                :align-top #f
                                :align-left #f
                                :paint-border #t
                                :border-rounding 2
                                :scale-font-size #t
                                :cut-text-to-fit #f
                                )

  (define (get-text)
    (if (procedure? text)
        (text)
        text))

  (define (get-text-color)
    (if (procedure? text-color)
        (text)
        text-color))

  (define (get-background-color)
    (and background-color
         (maybe-thunk-value background-color)))
        
  (define-override (paint)
    (let ((background-color (get-background-color)))
      (if background-color
          (<gui> :filled-box gui background-color x1 y1 x2 y2 border-rounding border-rounding)))
    
    (<gui> :draw-text gui (maybe-thunk-value text-color) (maybe-thunk-value text)
           (+ (if align-left
                  (+ 2 x1)
                  x1)
              1)
           y1
           (- x2 1)
           y2
           wrap-lines
           align-top
           align-left
           0 ;; rotate
           cut-text-to-fit
           scale-font-size
           )

    (when paint-border
      (define background-color (<gui> :get-background-color gui))
      (<gui> :draw-box gui background-color (+ 0 x1) (+ 0 y1) (- x2 0) (- y2 0) 2 0 0)
      (<gui> :draw-box gui *mixer-strip-border-color* x1 y1 x2 y2 1.5 border-rounding border-rounding))
    
    )
  )

(define (get-default-button-color gui)
  (define gui-background-color (<gui> :get-background-color gui))
  (<gui> :mix-colors "#010101" gui-background-color 0.5))

(def-area-subclass (<checkbox> :gui :x1 :y1 :x2 :y2
                               :is-selected-func
                               :value-changed-callback
                               :paint-func #f
                               :text "" ;; Only used if paint-func is #f
                               :text-color *text-color* ;; Only used if paint-func is #f
                               :selected-color #f ;; only used if paint-func is #f. If #f, use get-default-button-color
                               :right-mouse-clicked-callback #f
                               :border-width 0.25
                               :box-rounding #f
                               )

  (if (not selected-color)
      (set! selected-color (get-default-button-color gui)))

  (define-override (paint)
    (if paint-func
        (paint-func (is-selected-func))
        (draw-checkbox gui
                       text
                       (is-selected-func)
                       x1 y1 x2 y2
                       selected-color
                       :text-color text-color
                       :paint-implicit-border #t
                       :implicit-border-width border-width
                       :box-rounding box-rounding
                       )))

  (add-mouse-cycle! (lambda (button x* y*)
                      (cond ((and right-mouse-clicked-callback
                                  (= button *right-button*)
                                  (not (<ra> :shift-pressed)))
                             (right-mouse-clicked-callback)
                             #t)
                            ((= button *left-button*)                        
                             (value-changed-callback (not (is-selected-func)))
                             (update-me!)
                             #t)
                            (else
                             #t)))
                    (lambda (button x* y*)
                      #f)
                    (lambda (button x* y*)
                      #f)))
                          
                            
                      

(def-area-subclass (<button> :gui :x1 :y1 :x2 :y2
                             :paint-func #f
                             :text ""
                             :background-color #f
                             :statusbar-text #f
                             :callback #f
                             :callback-release #f
                             :right-mouse-clicked-callback #f)
  

  (define is-pressing #f)

  (define fontheight (get-fontheight))
  (define b (max 1 (myfloor (/ fontheight 2.5)))) ;; border
  
  (define r 3) ;;rounding
  (define r/2 2)

  (if (not background-color)
      (set! background-color (get-default-button-color gui)))
  
  (define (mypaint)
    (if (not is-pressing)
        (<gui> :filled-box gui background-color (+ x1 0) (+ y1 0) (- x2 0) (- y2 0) r r))
    
    (if (not (string=? "" text))
        (<gui> :draw-text
               gui
               *text-color*
               text
               (+ x1 3) (+ y1 2) (- x2 3) (- y2 2)
               #t ; wrap lines
               #f ; align left
               #f ; align top
               0  ; rotate
               #f ; cut text to fit
               #t ; scale font size
               ))
    (if is-pressing
        (<gui> :draw-box gui background-color (+ x1 r/2) (+ y1 r/2) (- x2 r/2) (- y2 r/2) b r r)))
    
  (define-override (paint)
    (if paint-func
        paint-func
        (mypaint)))

  (if statusbar-text
      (add-statusbar-text-handler statusbar-text))
  
  (add-mouse-cycle! (lambda (button x* y*)
                      (cond ((and right-mouse-clicked-callback
                                  (= button *right-button*)
                                  (not (<ra> :shift-pressed)))
                             (right-mouse-clicked-callback)
                             #t)
                            ((= button *left-button*)
                             (set! is-pressing #t)
                             (if callback
                                 (callback))
                             (update-me!)
                             #t)
                            (else
                             #f)))
                    (lambda (button x* y*)
                      #t)
                    (lambda (button x* y*)
                      (set! is-pressing #f)
                      (cond ((and callback-release
                                  (= button *left-button*))
                             (callback-release)))
                      (update-me!))))


(def-area-subclass (<scrollbar> :gui :x1 :y1 :x2 :y2
                                :callback
                                :slider-length ;; between 0 and 1. E.g. for 0.5; slider length = scrollbar length * 0.5
                                :vertical
                                :background-color #f
                                :paint-border #t
                                :border-color "black"
                                :border-rounding 2
                                :slider-color "#400010"
                                :slider-pressed-color #f
                                )

  (if (not slider-pressed-color)
      (set! slider-pressed-color (<gui> :mix-colors "#fefefe" slider-color 0.1)))

  (define b (/ (get-fontheight) 5.0))

  (define slider-pos 0) ;; goes between 0 and 1.0 - slider-length

  (add-method! :set-slider-pos!
               (lambda (new-slider-pos call-callback?)
                 (set! slider-pos new-slider-pos)
                 (if call-callback?
                     (report!))
                 (update-me!)))
  
  (define is-moving #f)
  
  (define-override (paint)
    (if background-color
        (<gui> :filled-box gui background-color x1 y1 x2 y2 border-rounding border-rounding))
    
    (define sx1 (+ b x1))
    (define sy1 (+ b y1))
    (define sx2 (- x2 b))
    (define sy2 (- y2 b))
    (define sheight (- sy2 sy1))
    (define swidth (- sx2 sx1))
    
    (if vertical
        (begin
          (set! sy1 (scale slider-pos 0 1 sy1 sy2))
          (set! sy2 (+ sy1 (scale slider-length 0 1 0 sheight))))
        (begin
          (set! sx1 (scale slider-pos 0 1 sx1 sx2))
          (set! sx2 (+ sx1 (scale slider-length 0 1 0 swidth)))))

    (<gui> :filled-box gui
           (if is-moving
               slider-pressed-color
               slider-color)
           sx1 sy1 sx2 sy2 border-rounding border-rounding)
    
    (if paint-border
        (<gui> :draw-box gui slider-color x1 y1 x2 y2 b (* 2 border-rounding) (* 2 border-rounding)))
        
    #t)

  (define (report!)
    (callback slider-pos
	      (+ slider-pos slider-length)))

  (define start-mouse-pos 0)
  
  (add-delta-mouse-cycle!
   (lambda (button x* y*)
     (and (= button *left-button*)
          (begin
            ;;(c-display "start:" slider-pos)
            (set! start-mouse-pos slider-pos)
            (set! is-moving #t)
            (update-me!)
            #t)))
   (lambda (button x* y* dx dy)
     (set! slider-pos (between 0
                               (+ start-mouse-pos
                                  (scale (if vertical dy dx)
                                         0 (if vertical
                                               (- height (* b 2))
                                               (- width (* b 2)))
                                         0 1))
                               (- 1.0 slider-length)))
     ;;(c-display "move"
     ;;           :slider-pos slider-pos
     ;;           :start-mouse-pos start-mouse-pos
     ;;           :scale (scale (if vertical dy dx)
     ;;                         0 (if vertical height width)
     ;;                         0 1))
     (report!)
     (update-me!)
     )
   (lambda (button x* y*)
     (set! is-moving #f)
     (update-me!)
     ;;(c-display "release button/x/y" x* y*)
     #f
     ))

  (add-nonpress-mouse-cycle!
   :enter-func (lambda (x* y)
                 (c-display "ENTER DRAGGER" class-name)
                 (set-mouse-pointer ra:set-open-hand-mouse-pointer gui)
                 #t)
   :leave-func (lambda ()
                 (c-display "LEAVE DRAGGER")
                 (set-mouse-pointer ra:set-normal-mouse-pointer gui)
                 #f))

  )

#!!
(begin
  (define testarea (make-qtarea))
  (define scrollbar (<new> :scrollbar (testarea :get-gui)
                           10 10 100 100
                           (lambda x (c-display "callback:" x))
                           0.2
                           #f
                           :background-color "white"
                           ))
  (testarea :add-sub-area-plain! scrollbar)
  (<gui> :show (testarea :get-gui)))
!!#

(def-area-subclass (<vertical-list-area> :gui :x1 :y1 :x2 :y2
                                         :areas
                                         :scrollbar-color "#400010"
                                         :background-color #f
                                         :expand-area-widths #t
                                         )

  (define scrollbar-width (between 1
                                   (/ width 10)
                                   (min (get-fontheight)
                                        (/ width 2))))

  (define scrollbar-x1 (- x2 scrollbar-width))

  (define total-area-height (apply + (map (lambda (area)
                                            (area :get-position (lambda (x1 y1 x2 y2 width height)
                                                                  height)))
                                          areas)))
  
  (define slider-length (if (= 0 total-area-height)
                            1
                            (min 1 (/ height total-area-height))))
  

  (define (scrollbar-callback pos1 pos2)
    (define pos1 (scale pos1
                        0 1
                        0 total-area-height))
    (position-areas! (+ y1 (- pos1))))

  (define scrollbar (<new> :scrollbar
                           gui
                           scrollbar-x1 y1
                           x2 y2
                           scrollbar-callback
                           :slider-length slider-length
                           :vertical #t
                           :background-color background-color))
  
  ;(for-each (lambda (area)
  ;            (add-sub-area! area 0 0))
  ;          areas)

  (define (position-areas! start-y1)
    (remove-sub-areas!)
    (add-sub-area-plain! scrollbar)
    (let loop ((areas areas)
               (area-y1 start-y1))
      (when (not (null? areas))
        (define area (car areas))
        (define area-y2 (+ area-y1 (area :get-height)))
        (if expand-area-widths
            (area :set-position2! x1 area-y1 (- scrollbar-x1 1) area-y2)
            (area :set-position! x1 area-y1))
        (if (and (>= area-y2 y1)
                 (< area-y1 y2))
            (add-sub-area-plain! area))
        (loop (cdr areas)
              area-y2))))

  (position-areas! y1)

  (define-override (i-am-removed!)
    (<gui> :remove-mouse-wheel-callback gui))

  (<gui> :add-mouse-wheel-callback gui
         (lambda (is-up x* y*)
           (define is-down (not is-up))
           (define first-y1 (and (not (null? areas))
                                 ((car areas) :get-position
                                  (lambda (a-x1 a-y1 a-x2 a-y2 a-width a-height)
                                    a-y1))))

           (define last-y2 (and (not (null? areas))
                                ((last areas) :get-position
                                 (lambda (a-x1 a-y1 a-x2 a-y2 a-width a-height)
                                   a-y2))))

           (c-display "is-up:" is-up x* y* first-y1 last-y2)

           (call-with-exit
            (lambda (exit)
              (let loop ((areas areas)
                         (n 0))
                (when (not (null? areas))
                  (define area (car areas))
                  (area :get-position
                        (lambda (a-x1 a-y1 a-x2 a-y2 a-width a-height)
                          (define (doit dy)
                            (define new-first-y1 (+ first-y1 dy))
                            (define new-last-y2 (+ last-y2 dy))
                            (position-areas! new-first-y1)
                            (scrollbar :set-slider-pos!
                                       (scale y1
                                              new-first-y1
                                              new-last-y2
                                              0 1)
                                       #f)
                            (exit))
                          (if is-up
                              (when (>= a-y2 y1)
                                ;;(c-display "  " n " how-much1:" (- y1 a-y1))
                                (doit (- y1 a-y1)))
                              (when (< y1 a-y2)
                                ;;(c-display "  " n " how-much2:" (- a-y2 y1))
                                (define dy (- last-y2 y2))                                                
                                (when (> dy 0)
                                  (set! dy (min dy (- a-y2 y1)))
                                  (doit (- dy)))))))
                  (loop (cdr areas)
                        (1+ n)
                        )))))
                              
           #t))
  )

#!!
(begin
  ;(def-area-subclass (<text-area> :gui :x1 :y1 :x2 :y2 :text)
  ;  (define-override (paint)
  ;    (<gui> :draw-text gui *text-color* text (+ x1 (get-fontheight)) y1 x2 y2
  ;           #f ;wrap-lines 
  ;           #f ;align-top 
  ;           #t ;align-left
  ;           )
  ;    (<gui> :draw-box gui "black" x1 y1 x2 y2 1.5 2 2)))
      
  (define testarea (make-qtarea :width 150 :height 450))
  (define list-area (<new> :vertical-list-area (testarea :get-gui) 10 20 150 400
                           (map (lambda (i)
                                  (<new> :text-area (testarea :get-gui)
                                         10 0 100 (* 1.2 (get-fontheight))
                                         (<-> i ": hello")
                                         :align-left #t))
                                (iota 20))
                           ))
  (testarea :add-sub-area-plain! list-area)
  (<gui> :show (testarea :get-gui))
  )
(show-async-message :text "hello")

(<ra> :get-path "/tmpwef")

(<ra> :iterate-directory (<ra> :get-path "/home/kjetil") #t
      (lambda (is-final file-info)
        (if (and (not is-final)
                 (file-info :is-audiofile))
            (c-display "file-info:" file-info))
        #t))
(<ra> :iterate-directory "L3RtcA==" #f c-display)
!!#

(def-area-subclass (<file-browser> :gui :x1 :y1 :x2 :y2
                                   :path "")
  
  (define entries '())

  (define (update-areas!)
    (c-display "\n\n\n---------------------- num entries:" (length entries) "-----------------------\n\n\n")
    (remove-sub-areas!)

    (define list-area (<new> :vertical-list-area gui x1 y1 x2 y2
                             (map (lambda (entry)
                                    (<new> :text-area (testarea :get-gui)
                                           0 0 10 (* 1.2 (get-fontheight))
                                           (entry :filename)
                                           :align-left #t
                                           :scale-font-size #f
                                           :cut-text-to-fit #t
                                           ))
                                  entries)))
    (add-sub-area-plain! list-area)
    )

  (update-areas!)

  ( ;;<ra> :schedule 5 ;; wait a little bit so that the main area has enough time to be displayed properly. (in RELEASE mode, this didn't make a difference)
        (lambda ()
          (let ((last-time 0)
                (temp '()))
            (<ra> :iterate-directory path #t
                  (lambda (is-finished file-info)
                    ;;(c-display "file-info:" file-info)
                    (if (not is-finished)
                        (set! temp (cons file-info temp)))
                    (when (or is-finished
                              (> (time) (+ last-time 0.05)))
                      (set! entries (sort (append! entries temp)
                                          (lambda (a b)
                                            (string<? (a :filename) (b :filename)))))
                      (set! temp '())
                      (update-areas!)
                      (set! last-time (time))
                      )
                    #t)))
          #f))
  )

#!!
(begin
  ;(def-area-subclass (<text-area> :gui :x1 :y1 :x2 :y2 :text)
  ;  (define-override (paint)
  ;    (<gui> :draw-text gui *text-color* text (+ x1 (get-fontheight)) y1 x2 y2
  ;           #f ;wrap-lines 
  ;           #f ;align-top 
  ;           #t ;align-left
  ;           )
  ;    (<gui> :draw-box gui "black" x1 y1 x2 y2 1.5 2 2)))
      
  (define testarea (make-qtarea :width 450 :height 750))
  (define list-area (<new> :file-browser (testarea :get-gui) 10 20 400 700
                           :path (<ra> :get-home-path)))
  (testarea :add-sub-area-plain! list-area)
  (<gui> :show (testarea :get-gui))
  )
!!#

(delafina (horizontally-layout-areas :x1 :y1 :x2 :y2
                                     :args
                                     :x1-border 0
                                     :y1-border 0
                                     :x2-border 0
                                     :y2-border 0
                                     :spacing 0
                                     :callback)
  (define half-spacing (/ spacing 2))
  (define num-areas (length args))
  (let loop ((args args)
             (n 0))
    (when (not (null? args))
      (let ((arg (car args))
            (x1* (+ (scale n 0 num-areas (+ x1 x1-border) (- x2 x2-border))
                    (if (> n 0)
                        half-spacing
                        0)))
            (x2* (- (scale (1+ n) 0 num-areas (+ x1 x1-border) (- x2 x2-border))
                    (if (< n (- num-areas 1))
                        half-spacing
                        0))))
        (apply callback (append (if (list? arg) arg (list arg))
                                (list x1* (+ y1 y1-border)
                                      x2* (- y2 y2-border)))))
      (loop (cdr args)
            (1+ n)))))

(delafina (vertically-layout-areas :x1 :y1 :x2 :y2
                                   :args
                                   :x1-border 0
                                   :y1-border 0
                                   :x2-border 0
                                   :y2-border 0
                                   :spacing 0
                                   :callback)
  (define half-spacing (/ spacing 2))
  (define num-areas (length args))
  (let loop ((args args)
             (n 0))
    (when (not (null? args))
      (let ((arg (car args))
            (y1* (+ (scale n 0 num-areas (+ y1 y1-border) (- y2 y2-border))
                    (if (> n 0)
                        half-spacing
                        0)))                 
            (y2* (- (scale (1+ n) 0 num-areas (+ y1 y1-border) (- y2 y2-border))
                    (if (< n (- num-areas 1))
                        half-spacing
                        0))))
        (apply callback (append (if (list? arg) arg (list arg))
                                (list (+ x1 x1-border) y1*
                                      (- x2 x2-border) y2*))))
      (loop (cdr args)
            (1+ n)))))


