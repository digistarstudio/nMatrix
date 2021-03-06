///////////////////////////////////////////////////////////////////////////
// VividTree.cpp : Implementation of Class VividTree
///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2005 Jim Alsup  All rights reserved
//              email: ja.alsup@gmail.com
//
// License: This code is provided "as is" with no expressed or implied 
//          warranty. You may use, or derive works from this file without
//          any restrictions except those listed below.
//
//        - This original header must be kept in the derived work.
//
//        - If your derived work is distributed in any form, you must
//          notify the author via the email address above and provide a 
//          short description of the product and intended audience.  
//
//        - You may not sell this code or a derived version of it as part of 
//          a comercial code library. 
//
//        - Offering the author a free licensed copy of any end product 
//          using this code is not required, but does endear you with a 
//          bounty of good karma.
//
///////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "resource.h"
#include "VividTree.h"
#include "UTX.h"


#define _OWNER_DRAWN 1  // Set to 0 to use Windows draw code.  


void GradientFillRect( CDC *pDC, CRect &rect, COLORREF col_from, COLORREF col_to, bool vert_grad )
{
	TRIVERTEX        vert[2];
	GRADIENT_RECT    mesh;

	vert[0].x      = rect.left;
	vert[0].y      = rect.top;
	vert[0].Alpha  = 0x0000;
	vert[0].Blue   = GetBValue(col_from) << 8;
	vert[0].Green  = GetGValue(col_from) << 8;
	vert[0].Red    = GetRValue(col_from) << 8;

	vert[1].x      = rect.right;
	vert[1].y      = rect.bottom; 
	vert[1].Alpha  = 0x0000;
	vert[1].Blue   = GetBValue(col_to) << 8;
	vert[1].Green  = GetGValue(col_to) << 8;
	vert[1].Red    = GetRValue(col_to) << 8;

	mesh.UpperLeft  = 0;
	mesh.LowerRight = 1;

#if _MSC_VER >= 1300  // only VS7 and above has GradientFill as a pDC member
	pDC->GradientFill( vert, 2, &mesh, 1, vert_grad ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H );
#else
	GradientFill( pDC->m_hDC, vert, 2, &mesh, 1, vert_grad ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H );
#endif
}


VividTree::VividTree()
:m_ItemSelectedBk(RGB(170, 205, 245))
{
	m_gradient_bkgd_sel = RGB( 0x80, 0xa0, 0xff );	//Blueish
	m_gradient_bkgd_from = RGB( 0xff, 0xff, 0xff );	//White
	m_gradient_bkgd_to = RGB( 0xd5, 0xd5, 0xe0 );	//Light Greyish Blue
	m_bkgd_mode = BK_MODE_GRADIENT;
	m_bmp_tiled_mode = false;
	m_gradient_horz = true;

	//m_BoldFont.CreateFont(
	//14,                        // nHeight
	//0,                         // nWidth
	//0,                         // nEscapement
	//0,                         // nOrientation
	//FW_BOLD,                   // nWeight
	//FALSE,                     // bItalic
	//FALSE,                     // bUnderline
	//0,                         // cStrikeOut
	//ANSI_CHARSET,              // nCharSet
	//OUT_DEFAULT_PRECIS,        // nOutPrecision
	//CLIP_DEFAULT_PRECIS,       // nClipPrecision
	//DEFAULT_QUALITY,    // nQuality DEFAULT_QUALITY NONANTIALIASED_QUALITY PROOF_QUALITY
	//DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
	//0 /*_T("Segoe UI")*/);

	m_icon = NULL;
}

VividTree::~VividTree()
{
}


BEGIN_MESSAGE_MAP(VividTree, CTreeCtrl)
	ON_WM_ERASEBKGND()
#if _OWNER_DRAWN
	ON_WM_PAINT()
#endif
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnTvnItemexpanding)
//	ON_WM_CREATE()

END_MESSAGE_MAP()


BOOL VividTree::OnEraseBkgnd(CDC* pDC)
{
    // nothing to do here -- see OnPaint
	return TRUE;
}


#if _OWNER_DRAWN
void VividTree::OnPaint()
{
	CPaintDC dc(this);	// Device context for painting
	CDC dc_ff;			// Memory base device context for flicker free painting
	CBitmap bm_ff;		// The bitmap we paint into
	CBitmap *bm_old;
	CFont *font, *old_font;
	CFont fontDC;
	int old_mode;

	GetClientRect(&m_rect);
	SCROLLINFO scroll_info;
	// Determine window portal to draw into taking into account
	// scrolling position
	if ( GetScrollInfo( SB_HORZ, &scroll_info, SIF_POS | SIF_RANGE ) )
	{
		m_h_offset = -scroll_info.nPos;
		m_h_size = max( scroll_info.nMax+1, m_rect.Width());
	}
	else
	{
		m_h_offset = m_rect.left;
		m_h_size = m_rect.Width();
	}
	if ( GetScrollInfo( SB_VERT, &scroll_info, SIF_POS | SIF_RANGE ) )
	{
		if ( scroll_info.nMin == 0 && scroll_info.nMax == 100) 
			scroll_info.nMax = 0;
		m_v_offset = -scroll_info.nPos * GetItemHeight();
		m_v_size = max( (scroll_info.nMax+2)*((int)GetItemHeight()+1), m_rect.Height() );
	}
	else
	{
		m_v_offset = m_rect.top;
		m_v_size = m_rect.Height();
	}

	// Create an offscreen dc to paint with (prevents flicker issues)
	dc_ff.CreateCompatibleDC( &dc );
    bm_ff.CreateCompatibleBitmap( &dc, m_rect.Width(), m_rect.Height() );
    // Select the bitmap into the off-screen DC.
    bm_old = (CBitmap *)dc_ff.SelectObject( &bm_ff );
	// Default font in the DC is not the font used by the tree control, so grab it and select it in.
	font = GetFont();
	old_font = dc_ff.SelectObject( font );
	// We're going to draw text transparently.
	old_mode = dc_ff.SetBkMode( TRANSPARENT );

	DrawBackGround( &dc_ff );
	DrawItems( &dc_ff );

    // Now Blt the changes to the real device context - this prevents flicker.
    dc.BitBlt( m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(), &dc_ff, 0, 0, SRCCOPY);

	dc_ff.SelectObject( old_font );
	dc_ff.SetBkMode( old_mode );
    dc_ff.SelectObject( bm_old );


//	printx("Enter VividTree::OnPaint().\n");
}
#endif

void VividTree::DrawBackGround( CDC* pDC )
{
	BkMode mode = m_bkgd_mode;

	if ( mode == BK_MODE_BMP )
	{
		if ( !m_bmp_back_ground.GetSafeHandle() )
			mode = BK_MODE_GRADIENT;
	}
	if ( mode == BK_MODE_GRADIENT )
	{
		GradientFillRect( pDC, 
			CRect( m_h_offset, m_v_offset, m_h_size + m_h_offset, m_v_size + m_v_offset), 
			m_gradient_bkgd_from, m_gradient_bkgd_to, !m_gradient_horz );
	}
	else if ( mode == BK_MODE_FILL )
		pDC->FillSolidRect( m_rect, pDC->GetBkColor() ); 
	else if ( mode == BK_MODE_BMP )
	{
		BITMAP bm;
		CDC dcMem;
	      
		VERIFY(m_bmp_back_ground.GetObject(sizeof(bm), (LPVOID)&bm));
		dcMem.CreateCompatibleDC(NULL);
		CBitmap* bmp_old = (CBitmap*)dcMem.SelectObject( &m_bmp_back_ground ); 
		
		if ( m_bmp_tiled_mode ) 	// BitMap Tile Mode
		{
			for ( int y = 0; y <= (m_v_size / bm.bmHeight); y++ )
			{
				for ( int x = 0; x <= (m_h_size / bm.bmWidth); x++ )
				{
					pDC->BitBlt((x*bm.bmWidth) + m_h_offset, (y*bm.bmHeight) + m_v_offset,
						bm.bmWidth, bm.bmHeight, &dcMem, 0, 0, SRCCOPY);
				}
			}
		}
		else  // BITMAP Stretch Mode
		{
			pDC->StretchBlt( m_h_offset, m_v_offset, m_h_size, m_v_size, 
				&dcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY );
		}
		// CleanUp
		dcMem.SelectObject( bmp_old );
	}
	else
		ASSERT( 0 );  // Unknown BackGround Mode
}

void VividTree::GetTextRect(HTREEITEM hItem, CRect &rect, BOOL bMember)
{
	CRect wndRect;
//	GetWindowRect(&wndRect);
	GetItemRect(hItem, wndRect, FALSE);
	INT width = wndRect.Width();
	const INT nRightReserved = 40;

	if(bMember)
	{
		rect.left = 41;
		rect.right = width - 20;
	}
	else
	{
		rect.left = 22;
		rect.right = width - nRightReserved;
	}
}

void VividTree::DrawItems( CDC *pDC )
{
}


void VividTree::SetBitmapID( UINT id )
{
	// delete any existing bitmap
    if (m_bmp_back_ground.GetSafeHandle())
        m_bmp_back_ground.DeleteObject();
	// add in the new bitmap
    VERIFY( m_bmp_back_ground.LoadBitmap( id ) ) ; 
    m_bmp_back_ground.GetSafeHandle();
}

// Determine if a referenced item is visible within the control window
bool VividTree::ItemIsVisible( HTREEITEM item )
{
	HTREEITEM scan_item;
	scan_item = GetFirstVisibleItem();
	while ( scan_item != NULL )
	{
		if ( item == scan_item )
			return true;
		scan_item = GetNextVisibleItem( scan_item );
	}
	return false;
}


// For a given tree node return an ICON for display on the left side.
// This default implementation only returns one icon.
// This function is virtual and meant to be overriden by deriving a class
// from VividTree and supplying your own icon images. 
HICON VividTree::GetItemIcon(HTREEITEM item, BOOL *bUpload, BOOL *bDownload)		
{
	return m_icon;  // default implementation - overridable
}


// If the background is a bitmap, and a tree is expanded/collapsed we
// need to redraw the entire background because windows moves the bitmap
// up (on collapse) destroying the position of the background.
void VividTree::OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	//if ( GetBkMode() == VividTree::BK_MODE_BMP && ItemIsVisible( pNMTreeView->itemNew.hItem ) )
//		Invalidate(FALSE);  // redraw everything
//	RedrawWindow();
	//	UpdateWindow();
	*pResult = 0;
}

void VividTree::ToggleButton(HTREEITEM hItem)
{
	UINT state;

	if(ItemHasChildren(hItem))
	{
		Expand(hItem, TVE_TOGGLE);
	//	TreeView_Expand(GetSafeHwnd(), hItem, TVE_TOGGLE);

		Invalidate(FALSE); // Doesn't remove this.
	}
	else
	{
		state = GetItemState(hItem, TVIF_STATE);

		if(state & TVIS_EXPANDED)
			SetItemState(hItem, 0, TVIS_EXPANDED);
		else
			SetItemState(hItem, TVIS_EXPANDED, TVIS_EXPANDED);
	}
}

LRESULT VividTree::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if(/*message == WM_PRINTCLIENT ||*/ message == WM_PRINT)
	{
	//	if(lParam == PRF_NONCLIENT)
	//		return CTreeCtrl::WindowProc(message, wParam, lParam);

		CDC *pDC = GetWindowDC(), TargetDC;
		if(pDC)
		{
			CRect rect;
			GetWindowRect(rect);
			TargetDC.Attach((HDC)wParam);
			TargetDC.BitBlt(0, 0, rect.Width(), rect.Height(), pDC, 0, 0, SRCCOPY);
			TargetDC.Detach();
			ReleaseDC(pDC);
		}

		return 1;
	}

	return CTreeCtrl::WindowProc(message, wParam, lParam);
}


