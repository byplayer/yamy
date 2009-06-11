//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// keymap.h


#ifndef _FUNCTION_H
#  define _FUNCTION_H


class SettingLoader;
class Engine;
class FunctionParam;

///
class FunctionData
{
public:
  /// virtual destructor
  virtual ~FunctionData() = 0;
  ///
  virtual void load(SettingLoader *i_sl) = 0;
  ///
  virtual void exec(Engine *i_engine, FunctionParam *i_param) const = 0;
  ///
  virtual const _TCHAR *getName() const = 0;
  ///
  virtual tostream &output(tostream &i_ost) const = 0;
  ///
  virtual FunctionData *clone() const = 0;
};

/// stream output
extern tostream &operator<<(tostream &i_ost, const FunctionData *i_data);


// create function
extern FunctionData *createFunctionData(const tstring &i_name);

///
enum VKey
{
  VKey_extended = 0x100,			///
  VKey_released = 0x200,			///
  VKey_pressed  = 0x400,			///
};

/// stream output
extern tostream &operator<<(tostream &i_ost, VKey i_data);


///
enum ToWindowType
{
  ToWindowType_toBegin            = -2,		///
  ToWindowType_toMainWindow       = -2,		///
  ToWindowType_toOverlappedWindow = -1,		///
  ToWindowType_toItself           = 0,		///
  ToWindowType_toParentWindow     = 1,		///
};

/// stream output
extern tostream &operator<<(tostream &i_ost, ToWindowType i_data);

// get value of ToWindowType
extern bool getTypeValue(ToWindowType *o_type, const tstring &i_name);


///
enum GravityType
{
  GravityType_C = 0,				/// center
  GravityType_N = 1 << 0,			/// north
  GravityType_E = 1 << 1,			/// east
  GravityType_W = 1 << 2,			/// west
  GravityType_S = 1 << 3,			/// south
  GravityType_NW = GravityType_N | GravityType_W, /// north west
  GravityType_NE = GravityType_N | GravityType_E, /// north east
  GravityType_SW = GravityType_S | GravityType_W, /// south west
  GravityType_SE = GravityType_S | GravityType_E, /// south east
};

/// stream output
extern tostream &operator<<(tostream &i_ost, GravityType i_data);

/// get value of GravityType
extern bool getTypeValue(GravityType *o_type, const tstring &i_name);


/// enum MouseHookType is defined in hook.h
extern enum MouseHookType;

/// stream output
extern tostream &operator<<(tostream &i_ost, MouseHookType i_data);

/// get value of MouseHookType
extern bool getTypeValue(MouseHookType *o_type, const tstring &i_name);


///
enum MayuDialogType
{
  MayuDialogType_investigate = 0x10000,		/// 
  MayuDialogType_log         = 0x20000,		/// 
  MayuDialogType_mask        = 0xffff0000,	/// 
};

/// stream output
extern tostream &operator<<(tostream &i_ost, MayuDialogType i_data);

// get value of MayuDialogType
bool getTypeValue(MayuDialogType *o_type, const tstring &i_name);

  
///
enum ModifierLockType
{
  ModifierLockType_Lock0 = Modifier::Type_Lock0, /// 
  ModifierLockType_Lock1 = Modifier::Type_Lock1, /// 
  ModifierLockType_Lock2 = Modifier::Type_Lock2, /// 
  ModifierLockType_Lock3 = Modifier::Type_Lock3, /// 
  ModifierLockType_Lock4 = Modifier::Type_Lock4, /// 
  ModifierLockType_Lock5 = Modifier::Type_Lock5, /// 
  ModifierLockType_Lock6 = Modifier::Type_Lock6, /// 
  ModifierLockType_Lock7 = Modifier::Type_Lock7, /// 
  ModifierLockType_Lock8 = Modifier::Type_Lock8, /// 
  ModifierLockType_Lock9 = Modifier::Type_Lock9, /// 
};

///
enum ToggleType
{
  ToggleType_toggle	= -1, /// 
  ToggleType_off	= 0, /// 
  ToggleType_on		= 1, /// 
};

/// stream output
extern tostream &operator<<(tostream &i_ost, ToggleType i_data);
  
// get value of ShowCommandType
extern bool getTypeValue(ToggleType *o_type, const tstring &i_name);


/// stream output
extern tostream &operator<<(tostream &i_ost, ModifierLockType i_data);

// get value of ModifierLockType
extern bool getTypeValue(ModifierLockType *o_type, const tstring &i_name);


///
enum ShowCommandType
{
  ShowCommandType_hide			= SW_HIDE, /// 
  ShowCommandType_maximize		= SW_MAXIMIZE, /// 
  ShowCommandType_minimize		= SW_MINIMIZE, /// 
  ShowCommandType_restore		= SW_RESTORE, /// 
  ShowCommandType_show			= SW_SHOW, /// 
  ShowCommandType_showDefault		= SW_SHOWDEFAULT, /// 
  ShowCommandType_showMaximized		= SW_SHOWMAXIMIZED, /// 
  ShowCommandType_showMinimized		= SW_SHOWMINIMIZED, /// 
  ShowCommandType_showMinNoActive	= SW_SHOWMINNOACTIVE, /// 
  ShowCommandType_showNA		= SW_SHOWNA, /// 
  ShowCommandType_showNoActivate	= SW_SHOWNOACTIVATE, /// 
  ShowCommandType_showNormal		= SW_SHOWNORMAL, /// 
};

/// stream output
extern tostream &operator<<(tostream &i_ost, ShowCommandType i_data);
  
// get value of ShowCommandType
extern bool getTypeValue(ShowCommandType *o_type, const tstring &i_name);


///
enum TargetWindowType
{
  TargetWindowType_overlapped	= 0, /// 
  TargetWindowType_mdi		= 1, /// 
};

/// stream output
extern tostream &operator<<(tostream &i_ost, TargetWindowType i_data);
  
// get value of ShowCommandType
extern bool getTypeValue(TargetWindowType *o_type, const tstring &i_name);


///
enum BooleanType
{
  BooleanType_false	= 0, /// 
  BooleanType_true	= 1, /// 
};

/// stream output
extern tostream &operator<<(tostream &i_ost, BooleanType i_data);
  
// get value of ShowCommandType
extern bool getTypeValue(BooleanType *o_type, const tstring &i_name);


///
enum LogicalOperatorType
{
  LogicalOperatorType_or	= 0, /// 
  LogicalOperatorType_and	= 1, /// 
};

/// stream output
extern tostream &operator<<(tostream &i_ost, LogicalOperatorType i_data);
  
// get value of LogicalOperatorType
extern bool getTypeValue(LogicalOperatorType *o_type, const tstring &i_name);


///
enum WindowMonitorFromType
{
  WindowMonitorFromType_primary = 0, ///
  WindowMonitorFromType_current = 1, ///
};

// stream output
extern tostream &operator<<(tostream &i_ost, WindowMonitorFromType i_data);

// get value of WindowMonitorFromType
extern bool getTypeValue(WindowMonitorFromType *o_type, const tstring &i_name);


/// stream output
extern tostream &operator<<(tostream &i_ost,
			    const std::list<tstringq> &i_data);


/// string type expression
class StrExpr;


/// string type expression for function arguments
class StrExprArg
{
private:
  StrExpr *m_expr;
public:
  enum Type
  {
    Literal,
    Builtin,
  };
  StrExprArg();
  StrExprArg(const StrExprArg &i_data);
  StrExprArg(const tstringq &i_symbol, Type i_type);
  ~StrExprArg();
  StrExprArg &operator=(const StrExprArg &i_data);
  tstringq eval() const;
  static void setEngine(const Engine *i_engine);
};


/// stream output
tostream &operator<<(tostream &i_ost, const StrExprArg &i_data);


#endif // !_FUNCTION_H
