;;; mayu-mode.el --- mayu setting editing commands for Emacs

;;; Copyright (C) 2000-2005 TAGA Nayuta <nayuta@ganaware.org>

;; Author: TAGA Nayuta <nayuta@ganaware.org>
;; Maintainer: TAGA Nayuta <nayuta@ganaware.org>
;; Keywords: languages, faces

(require 'font-lock)

(defvar
  mayu-font-lock-keywords
  (let* ((warning-face 
	  (if (boundp 'font-lock-warning-face)
	      font-lock-warning-face
	    font-lock-function-name-face))
	 (preprocessor-face 
	  (if (boundp 'font-lock-builtin-face)
	      font-lock-builtin-face
	    font-lock-preprocessor-face))
	 (function-name-face 
	  (if (boundp 'font-lock-builtin-face)
	      font-lock-builtin-face
	    font-lock-function-name-face)))
    `((,(concat
	 "\\<\\("
	 "[AMCWS]-"
	 "\\|IC-"
	 ;;"\\|I-"
	 "\\|[INCSK]L-"
	 "\\|M[0-9]-"
	 "\\|L[0-9]-"
	 "\\|U-"
	 "\\|D-"
	 "\\|R-"
	 "\\|E[01]-"
	 "\\|MAX-"
	 "\\|MIN-"
	 "\\|MMAX-"
	 "\\|MMIN-"
	 "\\)"
	 ) . font-lock-keyword-face)
      ("#.*$" . font-lock-comment-face)
      ("/[^/\n]*/" . font-lock-string-face)
      ("\\\\$" . ,warning-face)
      (,(concat
	 "^\\s *\\<\\("
	 "key"
	 "\\|event\\s +\\(prefixed\\|after-key-up\\|before-key-down\\)"
	 "\\|keyseq"
	 "\\|def\\s +\\(key\\|alias\\|mod\\|sync\\|subst\\|option\\)"
	 "\\|mod"
	 "\\|keymap"
	 "\\|keymap2"
	 "\\|window"
	 "\\|include"
	 "\\|if"
	 "\\|define"
	 "\\|else"
	 "\\|elseif"
	 "\\|elsif"
	 "\\|endif"
	 "\\)\\>"
	 ) . ,preprocessor-face)
      (,(concat
	 "&\\("
	 "Default"
	 "\\|KeymapParent"
	 "\\|KeymapWindow"
	 "\\|KeymapPrevPrefix"
	 "\\|OtherWindowClass"
	 "\\|Prefix"
	 "\\|Keymap"
	 "\\|Sync"
	 "\\|Toggle"
	 "\\|EditNextModifier"
	 "\\|Variable"
	 "\\|Repeat"
	 "\\|Undefined"
	 "\\|Ignore"
	 "\\|PostMessage"
	 "\\|ShellExecute"
	 "\\|SetForegroundWindow"
	 "\\|LoadSetting"
	 "\\|VK"
	 "\\|Wait"
	 "\\|InvestigateCommand"
	 "\\|MayuDialog"
	 "\\|DescribeBindings"
	 "\\|HelpMessage"
	 "\\|HelpVariable"
	 "\\|WindowRaise"
	 "\\|WindowLower"
	 "\\|WindowMinimize"
	 "\\|WindowMaximize"
	 "\\|WindowHMaximize"
	 "\\|WindowVMaximize"
	 "\\|WindowHVMaximize"
	 "\\|WindowMove"
	 "\\|WindowMoveTo"
	 "\\|WindowMoveVisibly"
	 "\\|WindowClingToLeft"
	 "\\|WindowClingToRight"
	 "\\|WindowClingToTop"
	 "\\|WindowClingToBottom"
	 "\\|WindowClose"
	 "\\|WindowToggleTopMost"
	 "\\|WindowIdentify"
	 "\\|WindowSetAlpha"
	 "\\|WindowRedraw"
	 "\\|WindowResizeTo"
	 "\\|WindowMonitor"
	 "\\|WindowMonitorTo"
	 "\\|MouseMove"
	 "\\|MouseWheel"
	 "\\|ClipboardChangeCase"
	 "\\|ClipboardUpcaseWord"
	 "\\|ClipboardDowncaseWord"
	 "\\|ClipboardCopy"
	 "\\|EmacsEditKillLinePred"
	 "\\|EmacsEditKillLineFunc"
	 "\\|LogClear"
	 "\\|DirectSSTP"
	 "\\|PlugIn"
	 "\\|Recenter"
	 "\\|SetImeStatus"
	 "\\|SetImeString"
	 "\\)\\>"
	 ) . ,function-name-face)
      "Default font-lock-keywords for mayu mode.")))

(defvar mayu-mode-syntax-table nil
  "syntax table used in mayu mode")
(setq mayu-mode-syntax-table (make-syntax-table))
(modify-syntax-entry ?# "<\n" mayu-mode-syntax-table)
(modify-syntax-entry ?\n ">#" mayu-mode-syntax-table)

(defvar mayu-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map "\C-c\C-c" 'comment-region)
    map))

;;;###autoload
(defun mayu-mode ()
  "A major mode to edit mayu setting files."
  (interactive)
  (kill-all-local-variables)
  (use-local-map mayu-mode-map)

  (make-local-variable 'comment-start)
  (setq comment-start "# ")
  (make-local-variable 'comment-start-skip)
  (setq comment-start-skip "\\(^\\|\\s-\\);?#+ *")
  (make-local-variable 'comment-indent-function)
  (setq comment-indent-function 'mayu-comment-indent)
  (make-local-variable 'parse-sexp-ignore-comments)
  (setq parse-sexp-ignore-comments t)

  (make-local-variable	'font-lock-defaults)
  (setq major-mode 'mayu-mode
	mode-name "mayu"
	font-lock-defaults '(mayu-font-lock-keywords nil)
	)
  (set-syntax-table mayu-mode-syntax-table)
  (run-hooks 'mayu-mode-hook))

;;; derived from perl-mode.el
(defun mayu-comment-indent ()
  (if (and (bolp) (not (eolp)))
      0					;Existing comment at bol stays there.
    (save-excursion
      (skip-chars-backward " \t")
      (max (if (bolp)			;Else indent at comment column
 	       0			; except leave at least one space if
 	     (1+ (current-column)))	; not at beginning of line.
 	   comment-column))))

(provide 'mayu-mode)

;;; mayu-mode.el ends here
