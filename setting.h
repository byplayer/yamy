//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// setting.h


#ifndef _SETTING_H
#  define _SETTING_H


#  include "keymap.h"
#  include "parser.h"
#  include "multithread.h"
#  include <set>


/// this class contains all of loaded settings
class Setting
{
public:
  typedef std::set<tstringi> Symbols;		///
  typedef std::list<Modifier> Modifiers;	/// 
  
public:
  Keyboard m_keyboard;				///
  Keymaps m_keymaps;				///
  KeySeqs m_keySeqs;				///
  Symbols m_symbols;				///
  bool m_correctKanaLockHandling;		///
  bool m_sts4mayu;				///
  bool m_cts4mayu;				///
  unsigned int m_oneShotRepeatableDelay;	///

public:
  Setting()
    : m_correctKanaLockHandling(false),
      m_sts4mayu(false),
      m_cts4mayu(false),
      m_oneShotRepeatableDelay(0) { }
};


///
namespace Event
{
  ///
  extern Key prefixed;
  ///
  extern Key before_key_down;
  ///
  extern Key after_key_up;
  ///
  extern Key *events[];
}


///
class SettingLoader
{
#  define FUNCTION_FRIEND
#  include "functions.h"
#  undef FUNCTION_FRIEND
  
public:
  ///
  class FunctionCreator
  {
  public:
    const _TCHAR *m_name;			/// 
    FunctionData *m_creator;			/// 
  };

private:
  typedef std::vector<Token> Tokens;		///
  typedef std::vector<tstringi> Prefixes;	///
  typedef std::vector<bool> CanReadStack;	/// 
  
private:
  Setting *m_setting;				/// loaded setting
  bool m_isThereAnyError;			/// is there any error ?

  SyncObject *m_soLog;				/// guard log output stream
  tostream *m_log;				/// log output stream
  
  tstringi m_currentFilename;			/// current filename
  
  Tokens m_tokens;				/// tokens for current line
  Tokens::iterator m_ti;			/// current processing token

  static Prefixes *m_prefixes;			/// prefix terminal symbol
  static size_t m_prefixesRefCcount;		/// reference count of prefix

  Keymap *m_currentKeymap;			/// current keymap

  CanReadStack m_canReadStack;			/// for &lt;COND_SYMBOL&gt;

  Modifier m_defaultAssignModifier;		/** default
                                                    &lt;ASSIGN_MODIFIER&gt; */
  Modifier m_defaultKeySeqModifier;		/** default
                                                    &lt;KEYSEQ_MODIFIER&gt; */

private:
  bool isEOL();					/// is there no more tokens ?
  Token *getToken();				/// get next token
  Token *lookToken();				/// look next token
  bool getOpenParen(bool i_doesThrow, const _TCHAR *i_name); /// argument "("
  bool getCloseParen(bool i_doesThrow, const _TCHAR *i_name); /// argument ")"
  bool getComma(bool i_doesThrow, const _TCHAR *i_name); /// argument ","
  
  void load_LINE();				/// &lt;LINE&gt;
  void load_DEFINE();				/// &lt;DEFINE&gt;
  void load_IF();				/// &lt;IF&gt;
  void load_ELSE(bool i_isElseIf, const tstringi &i_token);
						/// &lt;ELSE&gt; &lt;ELSEIF&gt;
  bool load_ENDIF(const tstringi &i_token);	/// &lt;ENDIF&gt;
  void load_INCLUDE();				/// &lt;INCLUDE&gt;
  void load_SCAN_CODES(Key *o_key);		/// &lt;SCAN_CODES&gt;
  void load_DEFINE_KEY();			/// &lt;DEFINE_KEY&gt;
  void load_DEFINE_MODIFIER();			/// &lt;DEFINE_MODIFIER&gt;
  void load_DEFINE_SYNC_KEY();			/// &lt;DEFINE_SYNC_KEY&gt;
  void load_DEFINE_ALIAS();			/// &lt;DEFINE_ALIAS&gt;
  void load_DEFINE_SUBSTITUTE();		/// &lt;DEFINE_SUBSTITUTE&gt;
  void load_DEFINE_OPTION();			/// &lt;DEFINE_OPTION&gt;
  void load_KEYBOARD_DEFINITION();		/// &lt;KEYBOARD_DEFINITION&gt;
  Modifier load_MODIFIER(Modifier::Type i_mode, Modifier i_modifier,
			 Modifier::Type *o_mode = NULL);
						/// &lt;..._MODIFIER&gt;
  Key *load_KEY_NAME();				/// &lt;KEY_NAME&gt;
  void load_KEYMAP_DEFINITION(const Token *i_which);
						/// &lt;KEYMAP_DEFINITION&gt;
  void load_ARGUMENT(bool *o_arg);		/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(int *o_arg);		/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(unsigned int *o_arg);	/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(long *o_arg);		/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(tstringq *o_arg);		/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(std::list<tstringq> *o_arg); /// &lt;ARGUMENT&gt;
  void load_ARGUMENT(tregex *o_arg);		/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(VKey *o_arg);		/// &lt;ARGUMENT_VK&gt;
  void load_ARGUMENT(ToWindowType *o_arg);	/// &lt;ARGUMENT_WINDOW&gt;
  void load_ARGUMENT(GravityType *o_arg);	/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(MouseHookType *o_arg);	/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(MayuDialogType *o_arg);	/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(ModifierLockType *o_arg);	/// &lt;ARGUMENT_LOCK&gt;
  void load_ARGUMENT(ToggleType *o_arg);	/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(ShowCommandType *o_arg);	///&lt;ARGUMENT_SHOW_WINDOW&gt;
  void load_ARGUMENT(TargetWindowType *o_arg);
					/// &lt;ARGUMENT_TARGET_WINDOW_TYPE&gt;
  void load_ARGUMENT(BooleanType *o_arg);	/// &lt;bool&gt;
  void load_ARGUMENT(LogicalOperatorType *o_arg);/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(Modifier *o_arg);		/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(const Keymap **o_arg);	/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(const KeySeq **o_arg);	/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(StrExprArg *o_arg);	/// &lt;ARGUMENT&gt;
  void load_ARGUMENT(WindowMonitorFromType *o_arg);	/// &lt;ARGUMENT&gt;
  KeySeq *load_KEY_SEQUENCE(
    const tstringi &i_name = _T(""), bool i_isInParen = false,
    Modifier::Type i_mode = Modifier::Type_KEYSEQ); /// &lt;KEY_SEQUENCE&gt;
  void load_KEY_ASSIGN();			/// &lt;KEY_ASSIGN&gt;
  void load_EVENT_ASSIGN();			/// &lt;EVENT_ASSIGN&gt;
  void load_MODIFIER_ASSIGNMENT();		/// &lt;MODIFIER_ASSIGN&gt;
  void load_LOCK_ASSIGNMENT();			/// &lt;LOCK_ASSIGN&gt;
  void load_KEYSEQ_DEFINITION();		/// &lt;KEYSEQ_DEFINITION&gt;

  /// load
  void load(const tstringi &i_filename);

  /// is the filename readable ?
  bool isReadable(const tstringi &i_filename, int i_debugLevel = 1) const;

  /// get filename
  bool getFilename(const tstringi &i_name,
		   tstringi *o_path, int i_debugLevel = 1) const;

public:
  ///
  SettingLoader(SyncObject *i_soLog, tostream *i_log);

  /// load setting
  bool load(Setting *o_setting, const tstringi &i_filename = _T(""));
};


/// get home directory path
typedef std::list<tstringi> HomeDirectories;
extern void getHomeDirectories(HomeDirectories *o_path);


#endif // !_SETTING_H
