//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// keymap.h


#ifndef _KEYMAP_H
#  define _KEYMAP_H

#  include "keyboard.h"
#  include "function.h"
#  include <vector>


///
class Action
{
public:
  ///
  enum Type
  {
    Type_key,					///
    Type_keySeq,				///
    Type_function,				///
  };

private:
  Action(const Action &i_action);
  
public:
  Action() { }
  ///
  virtual ~Action() { }
  ///
  virtual Type getType() const = 0;
  /// create clone
  virtual Action *clone() const = 0;
  /// stream output
  virtual tostream &output(tostream &i_ost) const = 0;
};

///
tostream &operator<<(tostream &i_ost, const Action &i_action);

///
class ActionKey : public Action
{
public:
  ///
  const ModifiedKey m_modifiedKey;

private:
  ActionKey(const ActionKey &i_actionKey);
  
public:
  ///
  ActionKey(const ModifiedKey &i_mk);
  ///
  virtual Type getType() const;
  /// create clone
  virtual Action *clone() const;
  /// stream output
  virtual tostream &output(tostream &i_ost) const;
};


class KeySeq;
///
class ActionKeySeq : public Action
{
public:
  KeySeq * const m_keySeq;			///

private:
  ActionKeySeq(const ActionKeySeq &i_actionKeySeq);
  
public:
  ///
  ActionKeySeq(KeySeq *i_keySeq);
  ///
  virtual Type getType() const;
  /// create clone
  virtual Action *clone() const;
  /// stream output
  virtual tostream &output(tostream &i_ost) const;
};


///
class ActionFunction : public Action
{
public:
  FunctionData * const m_functionData;		/// function data
  const Modifier m_modifier;			/// modifier for &Sync

private:
  ActionFunction(const ActionFunction &i_actionFunction);
  
public:
  ///
  ActionFunction(FunctionData *i_functionData,
		 Modifier i_modifier = Modifier());
  ///
  virtual ~ActionFunction();
  ///
  virtual Type getType() const;
  /// create clone
  virtual Action *clone() const;
  /// stream output
  virtual tostream &output(tostream &i_ost) const;
};


///
class KeySeq
{
public:
  typedef std::vector<Action *> Actions;	/// 

private:
  Actions m_actions;				///
  tstringi m_name;				///
  Modifier::Type m_mode;			/** Either Modifier::Type_KEYSEQ
                                                    or Modifier::Type_ASSIGN */

private:
  ///
  void copy();
  ///
  void clear();
  
public:
  ///
  KeySeq(const tstringi &i_name);
  ///
  KeySeq(const KeySeq &i_ks);
  ///
  ~KeySeq();
  
  ///
  const Actions &getActions() const { return m_actions; }
  
  ///
  KeySeq &operator=(const KeySeq &i_ks);
  
  /// add
  KeySeq &add(const Action &i_action);

  /// get the first modified key of this key sequence
  ModifiedKey getFirstModifiedKey() const;
  
  ///
  const tstringi &getName() const { return m_name; }
  
  /// stream output
  friend tostream &operator<<(tostream &i_ost, const KeySeq &i_ks);

  ///
  bool isCorrectMode(Modifier::Type i_mode) { return m_mode <= i_mode; }

  ///
  void setMode(Modifier::Type i_mode)
  {
    if (m_mode < i_mode)
      m_mode = i_mode;
    ASSERT( m_mode == Modifier::Type_KEYSEQ ||
	    m_mode == Modifier::Type_ASSIGN);
  }

  ///
  Modifier::Type getMode() const { return m_mode; }
};


///
class Keymap
{
public:
  ///
  enum Type
  {
    Type_keymap,				/// this is keymap
    Type_windowAnd,				/// this is window &amp;&amp;
    Type_windowOr,				/// this is window ||
  };
  ///
  enum AssignOperator
  {
    AO_new,					/// =
    AO_add,					/// +=
    AO_sub,					/// -=
    AO_overwrite,				/// !, !!
  };
  ///
  enum AssignMode
  {
    AM_notModifier,				///    not modifier
    AM_normal,					///    normal modifier
    AM_true,					/** !  true modifier(doesn't
                                                    generate scan code) */
    AM_oneShot,					/// !! one shot modifier
    AM_oneShotRepeatable,			/// !!! one shot modifier
  };
  
  /// key assignment
  class KeyAssignment
  {
  public:
    ModifiedKey m_modifiedKey;	///
    KeySeq *m_keySeq;		///

  public:
    ///
    KeyAssignment(const ModifiedKey &i_modifiedKey, KeySeq *i_keySeq)
      : m_modifiedKey(i_modifiedKey), m_keySeq(i_keySeq) { }
    ///
    KeyAssignment(const KeyAssignment &i_o)
      : m_modifiedKey(i_o.m_modifiedKey), m_keySeq(i_o.m_keySeq) { }
    ///
    bool operator<(const KeyAssignment &i_o) const
    { return m_modifiedKey < i_o.m_modifiedKey; }
  };

  /// modifier assignments
  class ModAssignment
  {
  public:
    AssignOperator m_assignOperator;	///
    AssignMode m_assignMode;		///
    Key *m_key;				///
  };
  typedef std::list<ModAssignment> ModAssignments; ///

  /// parameter for describe();
  class DescribeParam
  {
  private:
    typedef std::list<ModifiedKey> DescribedKeys;
    typedef std::list<const Keymap *> DescribedKeymap;
    friend class Keymap;

  private:
    DescribedKeys m_dk;
    DescribedKeymap m_dkeymap;
    bool m_doesDescribeModifiers;

  public:
    DescribeParam() : m_doesDescribeModifiers(true) { }
  };
  
private:
  /// key assignments (hashed by first scan code)
  typedef std::list<KeyAssignment> KeyAssignments;
  enum {
    HASHED_KEY_ASSIGNMENT_SIZE = 32,	///
  };

private:
  KeyAssignments m_hashedKeyAssignments[HASHED_KEY_ASSIGNMENT_SIZE];	///
  
  /// modifier assignments
  ModAssignments m_modAssignments[Modifier::Type_ASSIGN];

  Type m_type;					/// type
  tstringi m_name;				/// keymap name
  tregex m_windowClass;				/// window class name regexp
  tregex m_windowTitle;				/// window title name regexp

  KeySeq *m_defaultKeySeq;			/// default keySeq
  Keymap *m_parentKeymap;			/// parent keymap
  
private:
  ///
  KeyAssignments &getKeyAssignments(const ModifiedKey &i_mk);
  ///
  const KeyAssignments &getKeyAssignments(const ModifiedKey &i_mk) const;

public:
  ///
  Keymap(Type i_type,
	 const tstringi &i_name,
	 const tstringi &i_windowClass,
	 const tstringi &i_windowTitle,
	 KeySeq *i_defaultKeySeq,
	 Keymap *i_parentKeymap);
  
  /// add a key assignment;
  void addAssignment(const ModifiedKey &i_mk, KeySeq *i_keySeq);

  /// add modifier
  void addModifier(Modifier::Type i_mt, AssignOperator i_ao,
		   AssignMode i_am, Key *i_key);
  
  /// search
  const KeyAssignment *searchAssignment(const ModifiedKey &i_mk) const;

  /// get
  const KeySeq *getDefaultKeySeq() const { return m_defaultKeySeq; }
  ///
  Keymap *getParentKeymap() const { return m_parentKeymap; }
  ///
  const tstringi &getName() const { return m_name; }

  /// does same window
  bool doesSameWindow(const tstringi i_className,
		      const tstringi &i_titleName);
  
  /// adjust modifier
  void adjustModifier(Keyboard &i_keyboard);

  /// get modAssignments
  const ModAssignments &getModAssignments(Modifier::Type i_mt) const
  { return m_modAssignments[i_mt]; }

  /// describe
  void describe(tostream &i_ost, DescribeParam *i_dp) const;
  
  /// set default keySeq and parent keymap if default keySeq has not been set
  bool setIfNotYet(KeySeq *i_keySeq, Keymap *i_parentKeymap);
};


/// stream output
extern tostream &operator<<(tostream &i_ost, const Keymap *i_keymap);


///
class Keymaps
{
public:
  typedef std::list<Keymap *> KeymapPtrList;	/// 
  
private:
  typedef std::list<Keymap> KeymapList;		/// 

private:
  KeymapList m_keymapList;			/** pointer into keymaps may
                                                    exist */
  
public:
  ///
  Keymaps();
  
  /// search by name
  Keymap *searchByName(const tstringi &i_name);

  /// search window
  void searchWindow(KeymapPtrList *o_keymapPtrList,
		    const tstringi &i_className,
		    const tstringi &i_titleName);
  
  /// add keymap
  Keymap *add(const Keymap &i_keymap);
  
  /// adjust modifier
  void adjustModifier(Keyboard &i_keyboard);
};


///
class KeySeqs
{
private:
  typedef std::list<KeySeq> KeySeqList;		///

private:
  KeySeqList m_keySeqList;			///
  
public:
  /// add a named keyseq (name can be empty)
  KeySeq *add(const KeySeq &i_keySeq);
  
  /// search by name
  KeySeq *searchByName(const tstringi &i_name);
};


#endif // !_KEYMAP_H
