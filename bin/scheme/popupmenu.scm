(provide 'popupmenu.scm)

(my-require 'keybindings.scm)


;;;;;;;;;; popup menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define (parse-popup-menu-options args)
  ;;(c-display "\n\n\n>>----- args:")
  ;;(pretty-print args)
  ;;(newline)
  ;;(c-display "<<--------\n\n\n")
  (if (null? args)
      '()
      (cond ((and (list? (car args))
                  (not (null? (car args)))
                  (eq? (car (car args)) :radio-buttons))
             (let ((radiobuttons (car args)))
               ;;(c-display "REST:->>>" radiobuttons "<<<-")
               (append (list "[radiobuttons start]"
                             (lambda () #t))
                       (parse-popup-menu-options (cdr (car args)))
                       (list "[radiobuttons end]"
                             (lambda () #t))
                       (parse-popup-menu-options (cdr args)))))
             
            ((list? (car args))
             (parse-popup-menu-options (append (car args)
                                               (cdr args))))
            ((not (car args))
             (parse-popup-menu-options (cdr args)))
            
            ((string-starts-with? (car args) "---")
             (cons (car args)
                   (cons (lambda _ #t)
                         (parse-popup-menu-options (cdr args)))))
            (else
             (assert (not (null? (cdr args))))
             (let ((text (car args))
                   (arg2 (cadr args)))
               (cond ((eq? :check arg2)
                      (let ((check-on (caddr args)))
                        (parse-popup-menu-options (cons (<-> (if check-on "[check on]" "[check off]") text)
                                                        (cdddr args)))))                      
                     ((eq? :enabled arg2)
                      (let ((enabled (caddr args)))
                        (if enabled
                            (parse-popup-menu-options (cons text
                                                            (cdddr args)))
                            (parse-popup-menu-options (cons (<-> "[disabled]" text)
                                                            (cdddr args))))))
                     ((eq? :icon arg2)
                      (let ((filename (caddr args)))
                        ;;(c-display (<-> "stext: -" text "-" " rest:" (cdddr args)))
                        (parse-popup-menu-options (cons (<-> "[icon]" filename " " text)
                                                        (cdddr args)))))
                     ((eq? :shortcut arg2)
                      (let* ((shortcut (caddr args))
                             (keybinding (get-displayable-keybinding-from-shortcut shortcut)))
                        (if (not keybinding)
                            (parse-popup-menu-options (cons text
                                                            (cdddr args)))
                            (parse-popup-menu-options (cons (<-> "[shortcut]" keybinding "[/shortcut]" text)
                                                            (cdddr args))))))
                     ((procedure? arg2)
                      (let ((keybinding (get-displayable-keybinding (get-procedure-name arg2) '())))
                        (cons (if (not (string=? keybinding ""))
                                  (<-> "[shortcut]" keybinding "[/shortcut]" text)
                                  text)
                              (cons arg2
                                    (parse-popup-menu-options (cddr args))))))
                     ((list? arg2)
                      (append (list (<-> "[submenu start]" text)
                                    (lambda () #t))
                              (parse-popup-menu-options arg2)
                              (list "[submenu end]"
                                    (lambda () #t))
                              (parse-popup-menu-options (cddr args))))))))))

#!!

(parse-popup-menu-options (list                           
                           "?copytrack?"
                           :shortcut "aiai"
                           :enabled #f
                           (lambda () (c-display "clicked"))))

(parse-popup-menu-options (list                           
                           "?copytrack?"
                           :shortcut (lambda () 50)
                           (lambda () (c-display "clicked"))))

(parse-popup-menu-options (list                           
                           "?copytrack?"
                           :shortcut ra:copy-block
                           (lambda () (c-display "clicked"))))

(parse-popup-menu-options (list
                           (list
                            "?copytrack?"
                            :enabled #f
                            ra:copy-track)
                           (list
                            "?copytrack?2wefwefawefawefawef"
                            ra:copy-track)))

(popup-menu (list "?copytrack?"
                  :enabled #f
                  ra:copy-track)
            (list "?copytrack?2aewfas"
                  ra:copy-track))

(popup-menu (list "hello"
                  :shortcut (list ra:eval-scheme "(FROM_C-split-sample-seqblock-under-mouse #f)")
                  (lambda ()
                    2)))

(get-displayable-keybinding "" '())

(get-procedure-name ra:copy-track)

(documentation c-display)

(<ra> :get-html-from-text "<-rightjustify>")


(documentation ra:copy-track)
(documentation (lambda ()
                 (ra:copy-track)))


(parse-popup-menu-options (list 
                           "hello6"
                           :icon (<ra> :to-base64 "<<<<<<<<<<envelope_icon^Constant Power^fadein")
                           (lambda ()
                             (c-display "gakk4"))))
(<ra> :to-base64 "<<<<<<<<<<envelope_icon^Constant Power^fadein")

(parse-popup-menu-options (list "hello1" 
                                :enabled #f
                                :icon (<ra> :to-base64 "/home/kjetil/radium/temp/radium_64bit_linux-5.4.8/bin/radium_256x256x32.png")
                                (lambda ()
                                  6)))
(<ra> :to-base64 "/home/kjetil/radium/temp/radium_64bit_linux-5.4.8/bin/radium_256x256x32.png")

(parse-popup-menu-options (list (list "bbb" (lambda ()
                                              6))
                                "------"))

(parse-popup-menu-options (list "hello1" :enabled #t (lambda ()
                                                       (c-display "hepp1"))
                                "hello2" :enabled #f (lambda ()
                                                       (c-display "hepp2"))                                
                                "hello4" (lambda ()
                                           (c-display "hepp4"))))

(parse-popup-menu-options (list "hello1" :check #t (lambda ()
                                                     (c-display "hepp1"))                                
                                "hello4" (lambda ()
                                           (c-display "hepp4"))))

(parse-popup-menu-options (list "hello1" :check #f (lambda ()
                                                     (c-display "hepp1"))                                
                                "hello4" (lambda ()
                                           (c-display "hepp4"))))

(parse-popup-menu-options (list "hello1" (lambda ()
                                           (c-display "hepp1"))                                
                                "submenu" (list
                                           "hello2" (lambda ()
                                                      (c-display "hepp2"))
                                           "hello3" (lambda ()
                                                      (c-display "hepp3")))
                                "hello4" (lambda ()
                                           (c-display "hepp4"))))

!!#

(define (get-popup-menu-args args)
  (define options (parse-popup-menu-options args))
  ;;(c-display "bbb")
  ;;(c-display "optinos:" options)
  
  (define relations (make-assoc-from-flat-list options))
  (define strings (map car relations))
  ;;(define strings (list->vector (map car relations)))
  ;;
  ;;(define popup-arg (let loop ((strings (vector->list strings)))
  ;;                    ;;(c-display "strings" strings)
  ;;                    (if (null? strings)
  ;;                        ""
  ;;                        (<-> (car strings) " % " (loop (cdr strings))))))
  
  ;;(c-display "   relations: " relations)
  ;;(for-each c-display relations (iota (length relations)))
  ;;(c-display "strings: " strings)
  ;;(for-each c-display strings)
  ;;(c-display "popup-arg: " popup-arg)
  
  (define (get-func n)
    ;;(c-display "N: " n)
    ;;(define result-string (vector-ref strings n))
    ;;(cadr (assoc result-string relations))
    (cadr (relations n))
    )

  (list strings
        (lambda (n . checkboxval)
          (define result-string (strings n))
          (if (null? checkboxval)
              ((get-func n))
              ((get-func n) (car checkboxval))))))

(define (popup-menu-from-args popup-menu-args)
  ;;(c-display "ARGS:") (pretty-print popup-menu-args)
  (apply ra:popup-menu popup-menu-args)  
  )
        
;; Async only. Use ra:simple-popup-menu for sync.
(define (popup-menu . args)
  (popup-menu-from-args (get-popup-menu-args args)))


#!!
(popup-menu (list "Select"
                  :shortcut "Alt + B"
                  (lambda x
                    x))
            "------------"
            (list "Select2"
                  :shortcut ra:copy-block
                  (lambda x
                    x))
            "------------"
            (list "Select2"
                  :shortcut ra:copy-block
                  ra:copy-track)
            "------------"
            (list "Select2"
                  :shortcut "Alt + 7"
                  ra:copy-track)
            "------------"
            (list "Select2"
                  ra:copy-track)
            "------------"
            (list "Test2"
                  :shortcut (lambda () (c-display "hello"))
                  ra:copy-block))

(popup-menu (list (list "aaa" (lambda ()
                                5))
                  "----"
                  (list "bbb" (lambda ()
                                6))
                  "----------"))

(popup-menu "aaa" (lambda ()
                    (c-display "main menu"))
            "bbb" (list "aaa"
                        (lambda ()
                          (c-display "submenu"))))

(popup-menu "hello" :check #t (lambda (ison)
                                (c-display "gakk1" ison))
            "hello2" :enabled #t (lambda ()
                                   (c-display "gakk2"))
            "hello3" :enabled #f (lambda ()
                                   (c-display "gakk3"))
            (list 
             "hello4"
             :check #t
             :enabled #f
             (lambda (ison)
               (c-display "gakk4" ison)))
            (list 
             "hello5"
             :icon (<ra> :to-base64 "/home/kjetil/radium/temp/radium_64bit_linux-5.4.8/bin/radium_256x256x32.png")
             (lambda ()
               (c-display "gakk4" ison)))
            (list 
             "hello6"
             :icon (<ra> :to-base64 "<<<<<<<<<<envelope_icon^Constant Power^fadein")
             (lambda ()
               (c-display "gakk4" ison)))
            )
!!#
            
#||
(popup-menu "[check on] gakk1 on" (lambda (ison)
                                   (c-display "gakk1 " ison))
            "[check off] gakk2 off" (lambda (ison)
                                     (c-display "gakk2 " ison))
            "hepp1" (lambda ()
                     (c-display "hepp1"))
            "hepp2" (lambda ()
                     (c-display "hepp2"))
            )

(popup-menu "hello" (lambda ()
                      (c-display "hepp"))
            "[submenu start]Gakk gakk-" (lambda () #t)
            "[submenu start]Gakk gakk-" (lambda () #t)
            "hello2" (lambda ()
                       (c-display "hepp2"))
            "[submenu end]" (lambda () #t)
            "[submenu end]" (lambda () #t)
            "hepp" (lambda ()
                     (c-display "hepp3")))
(popup-menu "hello" (lambda ()
                      (c-display "hepp"))
            "Gakk gakk" (list
                         "Gakk gakk2" (list
                                       "hello2" (lambda ()
                                                  (c-display "hepp2"))
                                       "hello3" (lambda ()
                                                  (c-display "hepp3"))))
            "hepp" (lambda ()
                     (c-display "hepp3")))
||#

