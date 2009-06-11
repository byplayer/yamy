//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// layoutmanager.h


#ifndef _LAYOUTMANAGER_H
#  define _LAYOUTMANAGER_H

#  include "misc.h"
#  include <list>


///
class LayoutManager
{
public:
  ///
  enum Origin
  {
    ORIGIN_LEFT_EDGE,				/// 
    ORIGIN_TOP_EDGE = ORIGIN_LEFT_EDGE,		///
    ORIGIN_CENTER,				/// 
    ORIGIN_RIGHT_EDGE,				/// 
    ORIGIN_BOTTOM_EDGE = ORIGIN_RIGHT_EDGE,	/// 
  };

  ///
  enum Restrict
  {
    RESTRICT_NONE = 0,				/// 
    RESTRICT_HORIZONTALLY = 1,			/// 
    RESTRICT_VERTICALLY = 2,			/// 
    RESTRICT_BOTH = RESTRICT_HORIZONTALLY | RESTRICT_VERTICALLY, /// 
  };

private:
  ///
  class Item
  {
  public:
    HWND m_hwnd;				/// 
    HWND m_hwndParent;				/// 
    RECT m_rc;					/// 
    RECT m_rcParent;				/// 
    Origin m_origin[4];				/// 
  };

  ///
  class SmallestSize
  {
  public:
    HWND m_hwnd;				/// 
    SIZE m_size;				/// 

  public:
    ///
    SmallestSize() : m_hwnd(NULL) { }
  };

  typedef std::list<Item> Items;		/// 
  
protected:
  HWND m_hwnd;					/// 

private:
  Items m_items;				/// 
  Restrict m_smallestRestriction;		/// 
  SIZE m_smallestSize;				/// 
  Restrict m_largestRestriction;		/// 
  SIZE m_largestSize;				/// 
  
public:
  ///
  LayoutManager(HWND i_hwnd);
  
  /** restrict the smallest size of the window to the current size of it or
      specified by i_size */
  void restrictSmallestSize(Restrict i_restrict = RESTRICT_BOTH,
			    SIZE *i_size = NULL);
  
  /** restrict the largest size of the window to the current size of it or
      specified by i_size */
  void restrictLargestSize(Restrict i_restrict = RESTRICT_BOTH,
			   SIZE *i_size = NULL);
  
  ///
  bool addItem(HWND i_hwnd,
	       Origin i_originLeft = ORIGIN_LEFT_EDGE,
	       Origin i_originTop = ORIGIN_TOP_EDGE,
	       Origin i_originRight = ORIGIN_LEFT_EDGE,
	       Origin i_originBottom = ORIGIN_TOP_EDGE);
  ///
  void adjust() const;

  /// draw size box
  virtual BOOL wmPaint();

  /// size restriction
  virtual BOOL wmSizing(int i_edge, RECT *io_rc);

  /// hittest for size box
  virtual BOOL wmNcHitTest(int i_x, int i_y);

  /// WM_SIZE
  virtual BOOL wmSize(DWORD /* i_fwSizeType */, short /* i_nWidth */,
		      short /* i_nHeight */);
  
  /// forward message
  virtual BOOL defaultWMHandler(UINT i_message, WPARAM i_wParam,
				LPARAM i_lParam);
};


#endif // !_LAYOUTMANAGER_H
