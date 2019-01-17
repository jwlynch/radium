
(provide 'sequencer.scm)

(define (for-each-seqtracknum func)
  (let loop ((seqtracknum 0))
    (when (< seqtracknum (<ra> :get-num-seqtracks))
      (func seqtracknum)
      (loop (1+ seqtracknum)))))

  
(define (for-each-seqblocknum func)
  (define (for-each-seqblocknum2 seqtracknum func)
    (let loop ((seqblocknum 0))
      (when (< seqblocknum (<ra> :get-num-seqblocks seqtracknum))
        (func seqblocknum)
        (loop (1+ seqblocknum)))))
  
  (call-with-exit
   (lambda (return)
     (for-each-seqtracknum
      (lambda (seqtracknum)
        (for-each-seqblocknum2
         seqtracknum
         (lambda (seqblocknum)
           (define ret (func seqtracknum seqblocknum))
           (if (and (pair? ret) (pair? (cdr ret)) (eq? 'stop (car ret)) (null? (cddr ret)))
               (return (cadr ret)))))))
     (return #t))))

(define (map-all-seqblocks func)
  (let loop ((seqblocknum 0)
             (seqtracknum 0))
    (cond ((= seqtracknum (<ra> :get-num-seqtracks))
           '())
          ((= seqblocknum (<ra> :get-num-seqblocks seqtracknum)) ;; use-gfx))
           (loop 0 (1+ seqtracknum)))
          (else
           (cons (func seqtracknum seqblocknum)
                 (loop (1+ seqblocknum) seqtracknum))))))               

(define (for-each-selected-seqblock func)
  (for-each-seqblocknum (lambda (seqtracknum seqblocknum)
                          (when (<ra> :is-seqblock-selected seqblocknum seqtracknum)
                            ;;(c-display "funcing" seqtracknum seqblocknum)
                            (func seqtracknum seqblocknum)))))
  


;; see enum SeqtrackHeightType in nsmtracker.h
(define (get-seqtrack-height-type boxname)
  (cond ((eq? boxname 'custom) 0)
        ((eq? boxname '1-row) 1)
        ((eq? boxname '2-rows) 2)
        ((eq? boxname '3-rows) 3)
        ((eq? boxname 'unlimited) 4)
        (else
         (c-display "************** boxname:" boxname)
         (assert #f))))

(define (get-select-seqtrack-size-type-gui seqtracknum is-min gotit-callback)
  (define getter (if is-min ra:get-seqtrack-min-height-type ra:get-seqtrack-max-height-type))
  (define setter (if is-min ra:set-seqtrack-min-height-type ra:set-seqtrack-max-height-type))
  (define has-started #f)
  
  (define gui (<gui> :vertical-layout))
  
  (define (gotit type)    
    (when has-started
      (if #t
          (begin
            (setter seqtracknum type)
            (if gotit-callback
                (gotit-callback)
                (show-select-both-seqtrack-size-types-gui seqtracknum)))
          (<ra> :schedule 30
                (lambda ()
                  (eat-errors :try (lambda ()
                                      (setter seqtracknum type)
                                      (if gotit-callback
                                          (gotit-callback)))
                               :finally (lambda ()
                                          (<ra> :schedule 100
                                                (lambda ()
                                                  ;;(<gui> :close gui)
                                                  #f))))
                  #f)))))

  (define curr-min-type (<ra> :get-seqtrack-min-height-type seqtracknum))
  (define curr-max-type (<ra> :get-seqtrack-max-height-type seqtracknum))
  (define custom-type (get-seqtrack-height-type 'custom))
  (define unlimited-type (get-seqtrack-height-type 'unlimited))
    
  (for-each (lambda (name text)
              (define type (get-seqtrack-height-type
                            (if (or (eq? 'unlimited1 name)
                                    (eq? 'unlimited2 name))
                                'unlimited
                                name)))
              (set! text (cond ((eq? 'unlimited1 name)
                                "1/3 row")
                               ((eq? 'unlimited2 name)
                                "Unlimited")
                               (else
                                text)))
              (define is-disabled (or (and (not is-min)
                                           (eq? 'unlimited1 name))
                                      (and is-min
                                           (eq? 'unlimited2 name))))
              (if (and (not is-disabled)
                       (not (= type custom-type)))
                  (if is-min
                      (if (and (not (= unlimited-type type))
                               (not (= curr-max-type custom-type))
                               (> type curr-max-type))
                          (set! is-disabled #t))
                      (if (and (not (= curr-min-type unlimited-type))
                               (not (= curr-min-type custom-type))
                               (< type curr-min-type))
                          (set! is-disabled #t))))
              
              (define button (<gui> :radiobutton text (and (not is-disabled)
                                                           (= type (getter seqtracknum)))
                                    (lambda (val)
                                      (if val
                                          (gotit type)))))
              (<gui> :add gui button)
                
              (if is-disabled
                  (<gui> :set-enabled button #f)))
            '(unlimited1
              1-row
              2-rows
              3-rows
              ;;custom
              ;;unlimited2
              )
            '("Unlimited"
              "1 row"
              "2 rows"
              "3 rows"
              ;;"Current size"
              ;;"Unlimited"
              ))
  
  (set! has-started #t)

  gui)
  

(define *seqtrack-size-gui-uses-popup #f)

(define *seqtrack-size-gui-seqtracknum* -1)
(define (seqtrack-size-gui-open? seqtracknum)
  (= seqtracknum *seqtrack-size-gui-seqtracknum*))

(define (show-seqtrack-height-gui seqtracknum use-popup)

  (set! *seqtrack-size-gui-seqtracknum* seqtracknum)
  
  (define gui #f)
  
  (if (and (not use-popup)
           (not *seqtrack-size-gui-uses-popup))

      (begin
        (show-select-both-seqtrack-size-types-gui seqtracknum)
        ;;(<gui> :set-pos *curr-seqtrack-size-type-gui* (floor (<ra> :get-global-mouse-pointer-x)) (floor (<ra> :get-global-mouse-pointer-y)))
        (set! gui *curr-seqtrack-size-type-gui*)
        )
        
      (begin

        (set! gui (<gui> :popup))
        
        (define (gotit-callback)
          (<gui> :close gui)
          )
        
        (<gui> :add gui (get-select-seqtrack-size-type-gui seqtracknum #t gotit-callback))
        
        ;;(<gui> :set-modal gui #t)
        ;;(<gui> :set-pos gui (floor (<ra> :get-global-mouse-pointer-x)) (floor (<ra> :get-global-mouse-pointer-y)))
        
        ;;(<ra> :schedule 0 ;;100 ;; Add some delay to avoid mouse click not working the first time after closing the popup menu. (don't know what's happening)
          ;;    (lambda ()
                (<gui> :show gui)
                ;;    #f)))))
                ))

  (<gui> :add-deleted-callback gui
         (lambda (radium-runs-custom-exec)
           (c-display "DELETED")
           (set! *seqtrack-size-gui-seqtracknum* -1)
           (*sequencer-left-part-area* :update-me!)
           )))
        



(define *curr-seqtrack-size-type-gui* #f) ;; only show one at a time.
(define *curr-seqtrack-size-type-content* #f)

(define (show-select-both-seqtrack-size-types-gui seqtracknum)
  (set! *seqtrack-size-gui-seqtracknum* seqtracknum)
  (define min-gui (get-select-seqtrack-size-type-gui seqtracknum #t #f))
  (define max-gui (get-select-seqtrack-size-type-gui seqtracknum #f #f))
  (define header-text (<-> "               Seqtrack height for \"" (<ra> :get-seqtrack-name seqtracknum) "\" (#" seqtracknum ")               "))
  (define content (<gui> :vertical-layout
                         ;;(mid-horizontal-layout (<gui> :text header-text))
                         ;;(<gui> :horizontal-layout)
                         (if #t 
                             (<gui> :group header-text
                                    min-gui)
                             (<gui> :horizontal-layout
                                    (<gui> :group "Minimium size"
                                           min-gui)
                                    (<gui> :group "Maximum size"
                                           max-gui)))
                         ;;(<gui> :horizontal-layout)
                         ;;(mid-horizontal-layout (<gui> :text (<-> "(Note that this GUI operates on current seqtrack)")))
                         ))

  
  (<gui> :set-layout-spacing content 5 2 2 2 2)
  
  (<gui> :add content (<gui> :button "Close"
                             (lambda ()
                               (when *curr-seqtrack-size-type-gui*
                                 (<gui> :hide *curr-seqtrack-size-type-gui*)
                                 (set! *seqtrack-size-gui-seqtracknum* -1)))))

  (if (or (not *curr-seqtrack-size-type-gui*)
          (not (<gui> :is-open *curr-seqtrack-size-type-gui*)))
      (begin
        (set! *curr-seqtrack-size-type-gui* (<gui> :vertical-layout))
        (<gui> :set-window-title *curr-seqtrack-size-type-gui* "Seqtrack height limits")
        (<gui> :add *curr-seqtrack-size-type-gui* content)
        (<gui> :set-parent *curr-seqtrack-size-type-gui* (<gui> :get-sequencer-gui)))
      (begin
        (<gui> :replace *curr-seqtrack-size-type-gui* *curr-seqtrack-size-type-content* content)
        (<gui> :close *curr-seqtrack-size-type-content*)))

  (set! *curr-seqtrack-size-type-content* content)
  (<gui> :show *curr-seqtrack-size-type-gui*))


#!!
(show-select-both-seqtrack-size-types-gui 1)
!!#
                     
(define (select-seqtrack-size-type seqtracknum is-min)
  (define gui (get-select-seqtrack-size-type-gui seqtracknum is-min))
  (<gui> :set-parent gui (<gui> :get-sequencer-gui))
  (<gui> :show gui))
  

#!!
(select-seqtrack-size-type 0 #t)
!!#

(define (set-min-seqtrack-size seqtracknum)
  (select-seqtrack-size-type seqtracknum #t))

(define (set-max-seqtrack-size seqtracknum)
  (select-seqtrack-size-type seqtracknum #f))

(define (FROM_C-call-me-when-curr-seqtrack-has-changed seqtracknum)
  (if (and *curr-seqtrack-size-type-gui*
           (<gui> :is-open *curr-seqtrack-size-type-gui*)
           (<gui> :is-visible *curr-seqtrack-size-type-gui*))
      (show-select-both-seqtrack-size-types-gui seqtracknum)))

(define (get-nonstretched-seqblock-duration seqblocknum seqtracknum)
  (- (<ra> :get-seqblock-interior-end seqblocknum seqtracknum)
     (<ra> :get-seqblock-interior-start seqblocknum seqtracknum)))
      


;; see enum SeqblockBoxSelected in nsmtracker.h
(define (get-selected-box-num boxname)
  (cond ((eq? boxname 'non) 0)
        ((eq? boxname 'fade-left) 1)
        ((eq? boxname 'fade-right) 2)
        ((eq? boxname 'interior-left) 3)
        ((eq? boxname 'interior-right) 4)
        ((eq? boxname 'speed-left) 5)
        ((eq? boxname 'speed-right) 6)
        ((eq? boxname 'stretch-left) 7)
        ((eq? boxname 'stretch-right) 8)
        (else
         (c-display "************** boxname:" boxname)
         (assert #f))))
        


(define (get-interior-displayable-string value)
  (if (= value 0)
      "0.00s"
      (let ((seconds (/ value
                        (<ra> :get-sample-rate))))
        (if (< seconds 0.01)
            (let* ((ms (* 1000 seconds))                         
                   (sms (two-decimal-string ms)))
              (if (string=? sms "0.00")
                  "0.01ms"
                  (<-> sms "ms")))
            (<-> (two-decimal-string seconds) "s")))))

(define (get-left-interior-string2 value)
  (<-> "----|: " (get-interior-displayable-string value)))

(define (get-left-interior-string seqblocknum seqtracknum)
  (get-left-interior-string2 (<ra> :get-seqblock-interior-start seqblocknum seqtracknum #t)))

(define (left-interior-touched? seqblocknum seqtracknum)
  (let ((value (<ra> :get-seqblock-interior-start seqblocknum seqtracknum #t)))
    (not (= value 0.0))))

(define (set-left-interior-status-bar2 seqblocknum seqtracknum value)
  (if (and seqblocknum seqtracknum)
      (set-seqblock-selected-box 'interior-left seqblocknum seqtracknum))
  (set-editor-statusbar (get-left-interior-string2 value)))

(define (set-left-interior-status-bar seqblocknum seqtracknum)
  (set-left-interior-status-bar2 seqblocknum seqtracknum (<ra> :get-seqblock-interior-start seqblocknum seqtracknum #t)))

(define (get-right-interior-string2 seqblocknum seqtracknum right-interior-value)
  (<-> "|----: " (get-interior-displayable-string (- (<ra> :get-seqblock-default-duration seqblocknum seqtracknum)
                                                     right-interior-value))))

(define (get-right-interior-string seqblocknum seqtracknum)
  (get-right-interior-string2 seqblocknum seqtracknum (<ra> :get-seqblock-interior-end seqblocknum seqtracknum #t)))

#!!
(list :original-duration (<ra> :get-seqblock-default-duration 0 1)
      :resample-ratio (<ra> :get-seqblock-resample-ratio (<ra> :get-seqblock-id 0 1))
      :test (* (<ra> :get-sample-length (<ra> :get-seqblock-sample 0 1))
               (<ra> :get-seqblock-resample-ratio (<ra> :get-seqblock-id 0 1)))
      :test2 (* (<ra> :get-sample-length (<ra> :get-seqblock-sample 0 1))
                (/ 44100
                   96000.0))
      :interior-end (<ra> :get-seqblock-interior-end 0 1)
      :stretch-speed (<ra> :get-seqblock-stretch-speed (<ra> :get-seqblock-id 0 1))
      :sample-length (<ra> :get-sample-length (<ra> :get-seqblock-sample 0 1)))
!!#

(define (right-interior-touched? seqblocknum seqtracknum)
  (let ((value (<ra> :get-seqblock-interior-end seqblocknum seqtracknum #t)))
    (not (= value (<ra> :get-seqblock-default-duration seqblocknum seqtracknum)))))
  
(define (set-right-interior-status-bar2 seqblocknum seqtracknum right-interior-value)
  (set-seqblock-selected-box 'interior-right seqblocknum seqtracknum)
  (set-editor-statusbar (get-right-interior-string2 seqblocknum seqtracknum right-interior-value)))

(define (set-right-interior-status-bar seqblocknum seqtracknum)
  (set-right-interior-status-bar2 seqblocknum seqtracknum (<ra> :get-seqblock-interior-end seqblocknum seqtracknum #t)))

(define (get-speed-string2 value)
  (<-> "Speed: " (two-decimal-string (/ 1.0 value))))

(define (get-speed-string seqblockid)
  (get-speed-string2 (<ra> :get-seqblock-speed seqblockid #t)))

(define (speed-touched? seqblockid)
  (let ((speed (<ra> :get-seqblock-speed seqblockid #t)))
    (not (= speed 1.0))))

(define (get-stretch-string2 value)
  (<-> "Stretch: " (two-decimal-string value)))

(define (get-stretch-string seqblockid)
  (get-stretch-string2 (<ra> :get-seqblock-stretch seqblockid #t)))

(define (stretch-touched? seqblockid)
  (let ((stretch (<ra> :get-seqblock-stretch seqblockid #t)))
    (not (= stretch 1.0))))

(define (get-fade-string value seqblocknum seqtracknum)
  (<-> (if (= value 0)
           "0.00"
           (let* ((ms (* 1000
                         (/ (* value (- (<ra> :get-seqblock-end-time seqblocknum seqtracknum #t)
                                        (<ra> :get-seqblock-start-time seqblocknum seqtracknum #t)))
                            (<ra> :get-sample-rate))))
                  (sms (two-decimal-string ms)))
             (if (string=? sms "0.00")
                 "0.01"
                 sms)))
       "ms"))

(define (get-fade-string-left2 value seqblocknum seqtracknum)
  (<-> "Fade in: " (get-fade-string value seqblocknum seqtracknum)))

(define (get-fade-string-left seqblocknum seqtracknum)
  (let ((value (<ra> :get-seqblock-fade-in seqblocknum seqtracknum)))
    (get-fade-string-left2 value seqblocknum seqtracknum)))

(define (fade-left-touched? seqblocknum seqtracknum)
  (let ((value (<ra> :get-seqblock-fade-in seqblocknum seqtracknum)))
    (not (= value 0))))

(define (get-fade-string-right2 value seqblocknum seqtracknum)
  (<-> "Fade out: " (get-fade-string value seqblocknum seqtracknum)))

(define (get-fade-string-right seqblocknum seqtracknum)
  (let ((value (<ra> :get-seqblock-fade-out seqblocknum seqtracknum)))
    (get-fade-string-right2 value seqblocknum seqtracknum)))

(define (fade-right-touched? seqblocknum seqtracknum)
  (let ((value (<ra> :get-seqblock-fade-out seqblocknum seqtracknum)))
    (not (= value 0))))

(define (set-fade-status-bar is-left seqblocknum seqtracknum)
  (if is-left
      (begin
        (set-seqblock-selected-box 'fade-left seqblocknum seqtracknum)
        (set-editor-statusbar (get-fade-string-left seqblocknum seqtracknum)))
      (begin
        (set-seqblock-selected-box 'fade-right seqblocknum seqtracknum)
        (set-editor-statusbar (get-fade-string-right seqblocknum seqtracknum)))))

(define (get-seqtrack-popup-menu-entries seqtracknum)
  (list
   (list "Swap with next seqtrack"
         :enabled (< seqtracknum (- (<ra> :get-num-seqtracks) 1))
         (lambda ()
           (define (swapit)
             (<ra> :swap-seqtracks seqtracknum (1+ seqtracknum)))
           (if (and (= 0 seqtracknum)
                    (<ra> :seqtrack-for-audiofiles 1))
               (ask-user-about-first-audio-seqtrack
                (lambda (doit)
                  (if doit
                      (swapit))))
               (swapit))))
   (list "Set height"
         (lambda ()
           (show-select-both-seqtrack-size-types-gui seqtracknum)))))


#!!
(pp (<ra> :get-sequencer-tempos (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)))
(pp (<ra> :get-all-sequencer-tempos))
(pp (<ra> :get-all-sequencer-markers))

(pp (<ra> :get-sequencer-signatures (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)))
!!#


(define (get-sequencer-x-from-time time x1 x2)
  (scale time
         (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)
         x1 x2))

(define (get-time-triangle time area)
  (area :get-position
        (lambda (x1 y1 x2 y2 width height)
          (define tx (get-sequencer-x-from-time time x1 x2))
  
          (define h/2 (/ (- y2 y1) 2))
          (define triangle-x1 (- tx h/2))
          (define triangle (vector triangle-x1 (+ y1 1)
                                   (+ tx h/2) (+ 1 y1)
                                   tx y2))
          triangle)))

(define (get-triangle-x triangle)
  (triangle 4))

(define (inside-triangle? triangle x y)
  (define x1 (triangle 0))
  (define y1 (triangle 1))
  (define x2 (triangle 2))
  (define y2 (triangle 5))
  (and (>= x x1)
       (< x x2)
       (>= y y1)
       (< y y2)))

(define (show-sequencer-timing-help)
  (FROM-C-show-help-window "help/sequencer_timing.html"))

(define (get-sequencer-timing-popup-menu-entries)
  (list "Help timing"
        show-sequencer-timing-help))

(define *show-editor-timing-warning* #t)
(define *editor-timing-warning-is-visible* #f)

(define (show-editor-timing-warning)
  (define buttons '("Help" "Don't show this warning again" "OK"))
  (when (and *show-editor-timing-warning*
             (not *editor-timing-warning-is-visible*))
    (set! *editor-timing-warning-is-visible* #t)
    (show-async-message (<gui> :get-sequencer-gui)
                        "Tempos and signatures can not be edited in the sequencer timeline\nwhen timing is in \"editor timing mode\"."
                        :buttons buttons
                        :callback (lambda (option)
                                    (set! *editor-timing-warning-is-visible* #f)
                                    (cond ((string=? option (car buttons))
                                           (show-sequencer-timing-help))
                                          ((string=? option (cadr buttons))
                                           (set! *show-editor-timing-warning* #f)))))))
#!!
(show-editor-timing-warning)
!!#

(define (get-sequencer-time-from-x x x1 x2)
  (* 1.0 (max 0 (scale x x1 x2 (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)))))

(def-area-subclass (<sequencer-timeline-entry-area> :gui :x1 :y1 :x2 :y2
                                                    :value-type-name
                                                    :get-visible-entries
                                                    :get-all-entries
                                                    :set-all-entries!
                                                    :popup-menu*
                                                    :get-entry-info-string
                                                    :entry-color"#66ff0000"
                                                    :curr-entry-color "#660000ff"
                                                    :do-grid #f
                                                    :double-click-callback #f
                                                    :value->y #f
                                                    :is-tempo #f
                                                    :can-be-edited-in-sequencer-timing-mode #f
                                                    )
  
  (define curr-entry #f)
  (define curr-pos #f)
  (define curr-id #f)

  (define (update-parent!)
    (parent-area :update-me!))

  (define (remove-curr-entry!)
    (define entries (get-all-entries))
    (<ra> :undo-sequencer)
    (if (= 1 (vector-length entries))
        (set-all-entries! '())
        (begin
          (set! (entries curr-pos) (entries 0))
          (set-all-entries! (cdr (to-list entries)))))
    (<gui> :update (<gui> :get-sequencer-gui))
    (update-parent!))

  (add-method! :remove-curr-entry! remove-curr-entry!)

  (define (update-curr-entry-pos! entries curr-entry)
    (set! curr-pos (list-position entries (lambda (maybe) (morally-equal? maybe curr-entry))))
    (when (= curr-pos -1)
      (set! curr-pos 0)
      (c-display "curr-entry:" (pp curr-entry))
      (c-display "entries:" (pp (get-all-entries)))
      (assert (and "curr-pos<0" #f))))

  (define (sort-entries entries)
    (sort (to-list entries) (lambda (a b)
                              (< (a :time) (b :time)))))
  
  (define (replace-curr-entry2! entries curr-pos new-entry)
    (set! (entries curr-pos) new-entry)
    (set! entries (sort-entries entries))
    (set-all-entries! entries)
    (set! curr-entry new-entry)
    (set! curr-id (new-entry :uuid))
    (update-curr-entry-pos! entries new-entry)    
    (update-parent!))

  (define (replace-curr-entry! new-entry)
    (define entries (get-all-entries))
    (<ra> :undo-sequencer)
    (replace-curr-entry2! entries curr-pos new-entry))

  (add-method! :replace-curr-entry! replace-curr-entry!)

  (define (add-entry! new-entry)
    (define new-entries (sort-entries (cons new-entry
                                            (to-list (get-all-entries)))))
    (<ra> :undo-sequencer)
    (set-all-entries! new-entries)
    (update-curr-entry-pos! new-entries new-entry)
    (update-parent!))

  (add-method! :add-entry! add-entry!)

  ;; shift+right click
  (add-mouse-cycle! (lambda (button x* y*)
                      (and (= button *right-button*)
                           (<ra> :shift-pressed)
                           curr-entry
                           (begin
                             (if (and (not can-be-edited-in-sequencer-timing-mode)
                                      (not (<ra> :is-using-sequencer-timing)))
                                 (show-editor-timing-warning)
                                 (remove-curr-entry!))
                             #t))))
  ;; popup menu
  (if popup-menu*
      (add-mouse-cycle! (lambda (button x* y*)
                          (and (= button *right-button*)
                               (begin
                                 (popup-menu* x* curr-entry curr-pos)
                                 #t)))))

  ;; set song pos / double-clicking
  (let ()
    (define click-start-time 0)
    (add-mouse-cycle! (lambda (button x* y*)
                        (and (= button *left-button*)
                             (not curr-entry)
                             (begin
                               (c-display "HEPP")
                               (define time (<ra> :get-ms))
                               (if (and double-click-callback
                                        (< (- time click-start-time)
                                           (<ra> :get-double-click-interval)))
                                   (double-click-callback x* #f)
                                   (<ra> :set-song-pos (round (get-sequencer-time-from-x x* x1 x2))))
                               (set! click-start-time time))
                             #t))))
    
  ;; move entry
  (let ()
    (define curr-mouse-pos #f)
    (define curr-mouse-entry #f)
    (define mouse-entries #f)
    (define click-start-time 0)
    (define mouse-start-x #f)
    (define has-added-undo #f)
    (add-delta-mouse-cycle! (lambda (button x* y*)
                              (set! mouse-start-x x*)
                              (set! has-added-undo #f)
                              (c-display "Press2" x* y*)
                              (and (= button *left-button*)
                                   curr-entry
                                   (begin
                                     (set! curr-mouse-pos curr-pos)
                                     (set! mouse-entries (get-all-entries))
                                     (set! curr-mouse-entry curr-entry)
                                     (when double-click-callback
                                       (define time (<ra> :get-ms))
                                       (if (< (- time click-start-time)
                                              (<ra> :get-double-click-interval))
                                           (double-click-callback x* curr-entry)
                                           (set! click-start-time time)))
                                     #t)))
                            (lambda (button x* y* dx dy)
                              ;;(c-display "Move" x* y* dx dy "pos:" curr-pos)
                              (if (and (not can-be-edited-in-sequencer-timing-mode)
                                       (not (<ra> :is-using-sequencer-timing)))
                                  (show-editor-timing-warning)
                                  (begin
                                    (define new-time (get-sequencer-time-from-x (+ mouse-start-x dx) x1 x2))
                                    (if do-grid
                                        (if (not (<ra> :control-pressed))
                                            (set! new-time (* 1.0 (<ra> :get-seq-gridded-time (round new-time) (<ra> :get-seq-block-grid-type))))))
                                    (when (not has-added-undo)
                                      (<ra> :undo-sequencer)
                                      (set! has-added-undo #t))
                                    (replace-curr-entry2! mouse-entries curr-mouse-pos (copy-hash curr-mouse-entry :time new-time))
                                    (<gui> :update (<gui> :get-sequencer-gui)))))
                            (lambda (button x* y* dx dy)
                              ;;(c-display "Release" x* y* dx dy)
                              (if (and (= 0 dx)
                                       (= 0 dy))
                                  (<ra> :set-song-pos (round (curr-entry :time))))
                              (set! curr-id #f)
                              (update-parent!)                              
                              ;;(<gui> :tool-tip "")
                              )))


  ;; mark current entry
  (add-nonpress-mouse-cycle!
   :move-func
   (lambda (x* y*)
     (define had-curr-entry curr-entry)
     (set! curr-entry #f)
     (set! curr-id #f)
     (define entries (get-all-entries))
     ;;(c-display "Entries:" (pp entries))
     (let loop ((n 0))
       (if (< n (length entries))
           (begin
             (define entry (entries n))
             (define triangle (get-time-triangle (entry :time) this))
             (define triangle-x (get-triangle-x triangle))
             ;;(c-display "n:" n ". Entry:" entry "\ntriangle:" (map floor (to-list triangle)) ". Inside:" (inside-triangle? triangle x* y*) " .x/y:" x* y* "\n")
             (when (inside-triangle? triangle x* y*)
               (<gui> :tool-tip (<-> value-type-name ": " (get-entry-info-string entry #f #f)))
               (set! curr-entry entry)
               (set! curr-id (entry :uuid))
               (set! curr-pos n)
               (update-me!))
             (if (>= x* triangle-x)
                 (begin                   
                   (define next-entry (and (< (+ n 1) (length entries))
                                           (entries (+ n 1))))
                   (define next-time (and next-entry (next-entry :time)))
                   (define next-triangle-x (and next-time (get-sequencer-x-from-time next-time x1 x2)))
                   (define use-next-entry (and is-tempo
                                               next-entry
                                               (<= x* next-triangle-x)
                                               (= *logtype-linear* (entry :logtype))))
                                               
                   (<ra> :set-statusbar-text (<-> value-type-name ": "
                                                  (get-entry-info-string entry
                                                                         (and use-next-entry next-entry)
                                                                         (and use-next-entry (scale x* triangle-x next-triangle-x 0 100)))))))
             (loop (+ n 1)))))
     (when (and had-curr-entry
                (not curr-entry))
       (<gui> :tool-tip "")
       (update-parent!))
     #t)
   :leave-func
   (lambda (button-was-pressed)
     (when (and (not button-was-pressed)
                curr-entry)
       (set! curr-entry #f)
       (set! curr-id #f)
       (update-parent!))
     (when (not button-was-pressed)
       (<ra> :set-statusbar-text "")
       (<gui> :tool-tip ""))))
  
  
  (define paint-entries #f)

  (define (paint-background)

    ;;(<gui> :filled-box gui "sequencer_timeline_background_color" x1 y1 x2 y2 0 0 #f)
    ;;(<gui> :filled-box gui "sequencer_background_color" x1 y1 x2 y2 0 0 #f)

    (set! paint-entries (to-list (get-visible-entries)))

    (define (get-ty bpm)
      (scale bpm 0 200 y2 y1))

    (if is-tempo
        (<gui> :filled-polygon gui "#338811"
               (let loop ((entries paint-entries)
                          (last-y #f))
                 (if (null? entries)
                     (list x2 last-y
                           x2 y2)
                     (let ((entry (car entries)))
                       (define tx (get-sequencer-x-from-time (entry :time) x1 x2))
                       (define ty (get-ty (entry :bpm)))
                       (define rest2 (loop (cdr entries)
                                           ty))
                       (define rest (if (and (= (entry :logtype) *logtype-hold*)
                                             (not (null? (cdr entries))))
                                        (let ((next-entry (cadr entries)))
                                          (cons tx
                                                (cons ty
                                                      (cons (get-sequencer-x-from-time (next-entry :time) x1 x2)
                                                            (cons ty
                                                                  rest2)))))
                                        (cons tx
                                              (cons ty
                                                    rest2))))
                       (if (not last-y)
                           (cons tx
                                 (cons y2
                                       rest))
                           rest)))))))

  (add-method! :paint-background paint-background)

  (define-override (post-paint)

    (let loop ((entries (reverse paint-entries))
               (max-x2 10000000000000000000000)
               (prev-entry #f))
      (when (not (null? entries))
        (define entry (car entries))
        (define text (<-> (get-entry-info-string entry #f #f)
                          (cond ((or (not is-tempo)
                                     (not prev-entry)
                                     (= (entry :logtype) *logtype-hold*))
                                 "")
                                ((= (entry :bpm) (prev-entry :bpm))
                                 "⇒")
                                ((< (entry :bpm) (prev-entry :bpm))
                                 "⬀")
                                (else
                                 "⬂"))))
        (define tx (get-sequencer-x-from-time (entry :time) x1 x2))

        (define ty1 (if value->y
                        (value->y entry y1 y2)
                        (scale 100 0 200 y2 y1)))
        
        (define h/2 (/ (- y2 y1) 2))
        
        (define triangle (get-time-triangle (entry :time) this))

        (define triangle-x1 (triangle 0))

        (if (and curr-id
                 (string=? curr-id (entry :uuid)))
            (<gui> :filled-polygon gui curr-entry-color triangle)
            (<gui> :filled-polygon gui entry-color triangle))

        (<gui> :draw-polygon gui "black" triangle 0.7)

        (define text-width (<gui> :text-width text))
        (define text-x1 (+ h/2 tx))
        (define text-x2 (min max-x2 (+ text-x1 text-width)))

        (define ty ty1) ;;(get-ty (entry :bpm)))
        (define ym (/ (+ y1 y2) 2))
        (define b (/ (get-fontheight) 7))
        (<gui> :filled-box gui "#888888";;*text-color*
               (- tx b) (- ty b)
               (+ tx b) (+ ty b)
               ym ym)
        (<gui> :draw-box gui "black"
               (- tx b) (- ty b)
               (+ tx b) (+ ty b)
               1.4
               ym ym)

        (if (or (<= text-x1 tx) ;;(> text-x2 max-x2)
                (< (- text-x2 text-x1) 5))
            (loop (cdr entries)
                  triangle-x1
                  entry)
            (begin
              ;;(<gui> :filled-box gui "#80010101" tx1 y1 tx2 y2 5 5 #t)
              (<gui> :draw-line gui *text-color*
                     tx ty
                     (- text-x1 2) ym
                     2.0)
              (<gui> :draw-text
                     gui
                     *text-color*
                     ;;"black"
                     text
                     text-x1 y1 text-x2 y2
                     #f ; wrap lines
                     #f ; align left
                     #f ; align top
                     0  ; rotate
                     #f ; cut text to fit
                     #t ; scale font size
                     )
              (loop (cdr entries)
                    triangle-x1
                    entry
                    )))))
    
    ))


(define (bpm->string bpm)
  (if (morally-equal? bpm (floor bpm))
      (<-> (floor bpm))
      (one-decimal-string bpm)))

(define (create-sequencer-tempo-area gui x1 y1 x2 y2)
  (define area #f)

  (define (request-tempo x old-tempo callback)
    (<ra> :schedule 0
          (lambda ()
            (define time (get-sequencer-time-from-x x x1 x2))
            (define bpm (<ra> :request-float "BPM:" 1 100000 #t (<-> (if old-tempo (old-tempo :bpm) ""))))
            (and (>= bpm 1)
                 (callback (if old-tempo
                               (copy-hash old-tempo
                                          :time (old-tempo :time)
                                          :bpm bpm
                                          :logtype (old-tempo :logtype))
                               (hash-table* :time (get-sequencer-time-from-x x x1 x2)
                                            :bpm bpm
                                            :logtype *logtype-hold*
                                            :uuid (<ra> :create-uuid)
                                            ))))
            #f)))

  (set! area
        (<new> :sequencer-timeline-entry-area gui x1 y1 x2 y2
               :value-type-name "BPM"
               :get-visible-entries (lambda ()
                                      (<ra> :get-sequencer-tempos (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)))
               :get-all-entries (lambda ()
                                  (if (<ra> :is-using-sequencer-timing)
                                      (<ra> :get-all-sequencer-tempos)
                                      (<ra> :get-sequencer-tempos (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time))))
               :set-all-entries! ra:set-sequencer-tempos
               :popup-menu*
               (lambda (x* curr-tempo pos)
                 (popup-menu 
                  (list "Add tempo"
                        :enabled (and (not curr-tempo)
                                      (<ra> :is-using-sequencer-timing))
                        (lambda ()
                          (request-tempo x* #f                                         
                                         (lambda (tempo)
                                           (area :add-entry! tempo)))))
                  "-------------------"
                  (list "Remove tempo"
                        :enabled (and curr-tempo
                                      (<ra> :is-using-sequencer-timing))
                        (lambda ()
                          (area :remove-curr-entry!)))
                  (list "Set new tempo"
                        :enabled (and curr-tempo
                                      (<ra> :is-using-sequencer-timing))
                        (lambda ()
                          (request-tempo x* curr-tempo
                                         (lambda (tempo)
                                           (area :replace-curr-entry! tempo)))))
                  "-------------------"
                  (get-sequencer-timing-popup-menu-entries)

                  ;;(list "Glide to next tempo"
                  ;;      :enabled curr-tempo
                  ;;      :check (and curr-tempo
                  ;;                  (not (= *logtype-hold* (curr-tempo :logtype))))
                  ;;      (lambda (doglide)
                  ;;        (area :replace-curr-entry! (copy-hash curr-tempo :logtype (if doglide
                  ;;                                                                      *logtype-linear*
                  ;;                                                                      *logtype-hold*)))))
                  ))
               :get-entry-info-string
               (lambda (tempo next-tempo percentage-first)
                 (bpm->string (if next-tempo
                                  (scale percentage-first 0 100 (tempo :bpm) (next-tempo :bpm))
                                  (tempo :bpm))))
               :double-click-callback
               (lambda (x* curr-tempo)
                 (if (not (<ra> :is-using-sequencer-timing))
                     (show-editor-timing-warning)
                     (request-tempo x* curr-tempo
                                    (lambda (tempo)
                                      (if curr-tempo
                                          (area :replace-curr-entry! tempo)
                                          (area :add-entry! tempo))))))
               :value->y
               (lambda (tempo y1 y2)
                 (scale (tempo :bpm) 0 200 y2 y1))
               :is-tempo #t
               ))
  area)


(define (create-sequencer-signature-area gui x1 y1 x2 y2)
  (define area #f)

  (define (get-entry-info-string signature _a _b)
    (<-> (signature :numerator)
         "/"
         (signature :denominator)))

  (define (request-signature x old-signature callback)
    (<ra> :schedule 0
          (lambda ()
            (define time (get-sequencer-time-from-x x x1 x2))
            (define ratio (<ra> :request-string "Signature:" #t (if old-signature (get-entry-info-string old-signature #f #f) "")))
            (when (not (string=? "" ratio))
              (define numerator (<ra> :get-numerator-from-ratio-string ratio))
              (define denominator (<ra> :get-denominator-from-ratio-string ratio))
              (if (= 0 numerator)
                  (set! numerator 1))
              (callback (hash-table* :time (if old-signature
                                               (old-signature :time)
                                               time)
                                     :numerator numerator
                                     :denominator denominator
                                     :uuid (<ra> :create-uuid)
                                     )))
            #f)))

  (set! area
        (<new> :sequencer-timeline-entry-area gui x1 y1 x2 y2
               :value-type-name "Signature"
               :get-visible-entries (lambda ()
                                      (<ra> :get-sequencer-signatures (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)))
               :get-all-entries ra:get-all-sequencer-signatures
               :set-all-entries! ra:set-sequencer-signatures
               :popup-menu*
               (lambda (x* curr-signature pos)
                 (popup-menu 
                  (list "Add Signature"
                        :enabled (and (not curr-signature)
                                      (<ra> :is-using-sequencer-timing))
                        (lambda ()
                          (request-signature x* #f    
                                             (lambda (signature)
                                               (area :add-entry! signature)))))
                  "-------------------"
                  (list "Remove signature"
                        :enabled (and curr-signature
                                      (<ra> :is-using-sequencer-timing))
                        (lambda ()
                          (area :remove-curr-entry!)))
                  (list "Set new signature"
                        :enabled (and curr-signature
                                      (<ra> :is-using-sequencer-timing))
                        (lambda ()                                      
                          (request-signature x* curr-signature
                                             (lambda (signature)
                                               (area :replace-curr-entry! signature)))))
                  "-------------------"
                  (get-sequencer-timing-popup-menu-entries)                  
                  ))
               :get-entry-info-string get-entry-info-string
               :entry-color "#ccff8800"
               :curr-entry-color "#22ffff00"
               :double-click-callback
               (lambda (x* curr-signature)
                 (if (not (<ra> :is-using-sequencer-timing))
                     (show-editor-timing-warning)
                     (request-signature x* curr-signature
                                        (lambda (signature)
                                          (if curr-signature
                                              (area :replace-curr-entry! signature)
                                              (area :add-entry! signature))))))
               ))

  area)

(define (create-sequencer-marker-area gui x1 y1 x2 y2)
  (define area #f)

  (define (get-entry-info-string marker _a _b)
    (marker :name))

  (define (request-marker x old-marker callback)
    (<ra> :schedule 0
          (lambda ()
            (define time (get-sequencer-time-from-x x x1 x2))
            (define name (<ra> :request-string "Marker:" #t (if old-marker (get-entry-info-string old-marker #f #f) "")))
            (when (not (string=? "" name))
              (callback (hash-table* :time (if old-marker
                                               (old-marker :time)
                                               time)
                                     :name name
                                     :uuid (<ra> :create-uuid)
                                     )))
            #f)))

  (set! area
        (<new> :sequencer-timeline-entry-area gui x1 y1 x2 y2
               :value-type-name "Marker"
               :get-visible-entries ra:get-all-sequencer-markers
               :get-all-entries ra:get-all-sequencer-markers
               :set-all-entries! ra:set-sequencer-markers
               :popup-menu*
               (lambda (x* curr-marker pos)
                 (popup-menu 
                  (list "Add Marker"
                        :enabled (not curr-marker)
                        (lambda ()
                          (request-marker x* #f    
                                             (lambda (marker)
                                               (area :add-entry! marker)))))
                  "-------------------"
                  (list "Remove marker"
                        :enabled curr-marker
                        (lambda ()
                          (area :remove-curr-entry!)))
                  (list "Set new marker"
                        :enabled curr-marker
                        (lambda ()                                      
                          (request-marker x* curr-marker
                                             (lambda (marker)
                                               (area :replace-curr-entry! marker)))))
                  "-------------------"
                  (get-sequencer-timing-popup-menu-entries)
                  ))
               :entry-color "sequencer_marker_color" ;;"#66004488"
               :curr-entry-color "#ff002244"
               :get-entry-info-string get-entry-info-string
               :do-grid #t
               :double-click-callback
               (lambda (x* curr-marker)
                 (request-marker x* curr-marker
                                    (lambda (marker)
                                      (if curr-marker
                                          (area :replace-curr-entry! marker)
                                          (area :add-entry! marker)))))
               :can-be-edited-in-sequencer-timing-mode #t
               ))

  area)


#!!
(define (create-sequencer-upper-part-area gui x1 y1 x2 y2 state)
  ;;(c-display "    CREATE SEQUENCER UPPER AREA. State:" (pp state))
  (define (recreate gui x1 y1 x2 y2)
    (create-sequencer-tempo-area gui x1 y1 x2 y2))

  (recreate gui x1 y1 x2 y2))

(when (defined? '*sequencer-has-loaded*)
  (define testarea (make-qtarea :width (floor (- (<ra> :get-seqtimeline-area-x2) (<ra> :get-seqtimeline-area-x1))) :height (floor (* 1.2 (get-fontheight)))
                                :sub-area-creation-callback (lambda (gui width height state)
                                                               (create-sequencer-upper-part-area gui 0 0 width height state))))
  (<gui> :set-parent (testarea :get-gui) -1)
  (<gui> :show (testarea :get-gui)))
!!#


(define (get-signature-string signature)
  (<-> (signature :numerator) "/" (signature :denominator)))


(def-area-subclass (<sequencer-timeline-area> :gui :x1 :y1 :x2 :y2)
  (define tempo-y2 (scale 0.33 0 1 y1 y2))
  (define signature-y2 (scale 0.66 0 1 y1 y2))
  (add-sub-area-plain! (create-sequencer-tempo-area gui x1 y1 x2 tempo-y2))
  (add-sub-area-plain! (create-sequencer-signature-area gui x1 tempo-y2 x2 signature-y2))
  (add-sub-area-plain! (create-sequencer-marker-area gui x1 signature-y2 x2 y2))

  (define-override (paint)
    (<gui> :filled-box gui "sequencer_background_color" x1 y1 x2 y2 0 0 #f)

    (for-each (lambda (sub-area)
                (sub-area :paint-background))
              sub-areas)
  
    (define (draw-border y)
      (define y1 (floor y))
      (define y2 (+ y1 0.7))
      (<gui> :draw-line gui "black" x1 y1 x2 y1 1.2)
      (<gui> :draw-line gui "#bbffffff" x1 y2 x2 y2 0.7)
      )
    (draw-border tempo-y2)
    (draw-border signature-y2)

    (<ra> :iterate-sequencer-time (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time) "beat"
          (lambda (time barnum beatnum linenum)
            (define x (get-sequencer-x-from-time time x1 x2))
            (define color (if (= beatnum 1)
                              "#a0010101"
                              "#400101ff"))
            (<gui> :draw-line gui color x y1 x y2 (if (= beatnum 1) 1.1 1))
            #t
            ))

    )

  )
                       

(define (create-sequencer-upper-part-area gui x1 y1 x2 y2 state)
  ;;(c-display "    CREATE SEQUENCER UPPER AREA. State:" (pp state))
  (define (recreate gui x1 y1 x2 y2)
    (<new> :sequencer-timeline-area gui x1 y1 x2 y2))

  (recreate gui x1 y1 x2 y2))

(define *temp-test-area* (if (defined? '*temp-test-area*) *temp-test-area* #f))
(when (defined? '*sequencer-has-loaded*)
  (when *temp-test-area*
    (c-display "CLOSING" *temp-test-area*)
    (if (<gui> :is-open *temp-test-area*)
        (<gui> :close *temp-test-area*))
    (set! *temp-test-area* #f))
  (define qtarea (make-qtarea :width (floor (- (<ra> :get-seqtimeline-area-x2) (<ra> :get-seqtimeline-area-x1))) :height (floor (* 3.0 (get-fontheight)))
                              :sub-area-creation-callback (lambda (gui width height state)
                                                            (create-sequencer-upper-part-area gui 0 0 width height state))))
  (set! *temp-test-area* (qtarea :get-gui))
  (<gui> :set-parent *temp-test-area* -1)
  (<gui> :show *temp-test-area*))

(define *sequencer-has-loaded* #t)
