// DlgNetlist.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgNetlist.h"
#include "DlgEditNet.h"
#include "DlgSetTraceWidths.h"
#include ".\dlgnetlist.h"
#include "DlgNetCombine.h"

extern CFreePcbApp theApp;

// columns for list
enum {
	COL_VIS = 0,
	COL_NAME,
	COL_PINS,
	COL_WIDTH,
	COL_VIA_W,
	COL_HOLE_W
};

// sort types
enum {
	SORT_UP_NAME = 0,
	SORT_DOWN_NAME,
	SORT_UP_PINS,
	SORT_DOWN_PINS,
	SORT_UP_WIDTH,
	SORT_DOWN_WIDTH,
	SORT_UP_VIA_W,
	SORT_DOWN_VIA_W,
	SORT_UP_HOLE_W,
	SORT_DOWN_HOLE_W
};

// global so that it is available to Compare() for sorting list control items
netlist_info nli;

// global callback function for sorting
// lp1, lp2 are indexes to global arrays above
//		
int CALLBACK CompareNetlist( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	int ret = 0;
	switch( type )
	{
		case SORT_UP_NAME:
		case SORT_DOWN_NAME:
			ret = (strcmp( nli[lp1].name, nli[lp2].name ));
			break;

		case SORT_UP_WIDTH:
		case SORT_DOWN_WIDTH:
			if( nli[lp1].w > nli[lp2].w )
				ret = 1;
			else if( nli[lp1].w < nli[lp2].w )
				ret = -1;
			break;

		case SORT_UP_VIA_W:
		case SORT_DOWN_VIA_W:
			if( nli[lp1].v_w > nli[lp2].v_w )
				ret = 1;
			else if( nli[lp1].v_w < nli[lp2].v_w )
				ret = -1;
			break;

		case SORT_UP_HOLE_W:
		case SORT_DOWN_HOLE_W:
			if( nli[lp1].v_h_w > nli[lp2].v_h_w )
				ret = 1;
			else if( nli[lp1].v_h_w < nli[lp2].v_h_w )
				ret = -1;
			break;

		case SORT_UP_PINS:
		case SORT_DOWN_PINS:
			if( nli[lp1].ref_des.GetSize() > nli[lp2].ref_des.GetSize() )
				ret = 1;
			else if( nli[lp1].ref_des.GetSize() < nli[lp2].ref_des.GetSize() )
				ret = -1;
			break;
	}
	switch( type )
	{
		case SORT_DOWN_NAME:
		case SORT_DOWN_WIDTH:
		case SORT_DOWN_VIA_W:
		case SORT_DOWN_HOLE_W:
		case SORT_DOWN_PINS:
			ret = -ret;
			break;
	}

	return ret;
}

// CDlgNetlist dialog

IMPLEMENT_DYNAMIC(CDlgNetlist, CDialog)
CDlgNetlist::CDlgNetlist(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgNetlist::IDD, pParent)
{
	m_w = 0;
	m_v_w = 0;
	m_v_h_w = 0;
	m_nlist = 0;
}

CDlgNetlist::~CDlgNetlist()
{
}

void CDlgNetlist::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_NET, m_list_ctrl);
	DDX_Control(pDX, IDC_BUTTON_VISIBLE, m_button_visible);
	DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_edit);
	DDX_Control(pDX, IDC_BUTTON_DELETE, m_button_delete);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_button_add);
	DDX_Control(pDX, IDC_BUTTON_SELECT_ALL, m_button_select_all);
	DDX_Control(pDX, IDC_BUTTON_NL_WIDTH, m_button_nl_width);
	DDX_Control(pDX, IDC_BUTTON_COMBINE, m_button_combine);
	DDX_Control(pDX, IDOK, m_OK);
	DDX_Control(pDX, IDCANCEL, m_cancel);
	DDX_Control(pDX, IDC_BUTTON_DELETE2, m_button_delete_single);
	if( pDX->m_bSaveAndValidate )
	{
		// update nli with visibility data before leaving
		int n_local_nets = nli.GetSize();
		for( int iItem=0; iItem<m_list_ctrl.GetItemCount(); iItem++ )
		{
			int i = m_list_ctrl.GetItemData( iItem );
			nli[i].visible = ListView_GetCheckState( m_list_ctrl, iItem );
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgNetlist, CDialog)
//	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_NET, OnLvnItemchangedListNet)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_NET, OnLvnColumnclickListNet)
	ON_BN_CLICKED(IDC_BUTTON_VISIBLE, OnBnClickedButtonVisible)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL, OnBnClickedButtonSelectAll)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_DELETE2, OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_NL_WIDTH, OnBnClickedButtonNLWidth)
	ON_BN_CLICKED(IDC_BUTTON_COMBINE, OnBnClickedButtonCombine)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_NOPINS, OnBnClickedDeleteNetsWithNoPins)
	ON_NOTIFY(NM_CLICK, IDC_LIST_NET, OnNMClickListNet)
END_MESSAGE_MAP()


void CDlgNetlist::Initialize( cnetlist * nlist, cpartlist * plist,
		CArray<int> * w, CArray<int> * v_w, CArray<int> * v_h_w )
{
	m_nlist = nlist;
	m_plist = plist;
	m_w = w;
	m_v_w = v_w;
	m_v_h_w = v_h_w;
}

// CDlgNetlist message handlers

BOOL CDlgNetlist::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_nli = &nli;

	// make copy of netlist data so that it can be edited but user can still cancel
	// use global netlist_info so it can be sorted in the list control
	m_nlist->ExportNetListInfo( &nli );

	// initialize netlist control
	m_item_selected = -1;
	m_sort_type = 0;
	DrawListCtrl();

	// initialize buttons
	m_button_edit.EnableWindow(FALSE);
	m_button_delete_single.EnableWindow(FALSE);
	m_button_nl_width.EnableWindow(FALSE);
	m_button_delete.EnableWindow(FALSE);
	m_button_combine.EnableWindow(FALSE);
	return TRUE;  
}

// draw listview control and sort according to m_sort_type
//
void CDlgNetlist::DrawListCtrl()
{
	int nItem;
	CString str;
	DWORD old_style = m_list_ctrl.GetExtendedStyle();
	m_list_ctrl.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | LVS_EX_CHECKBOXES | old_style );
	m_list_ctrl.DeleteAllItems();
	CString colNames[6];
	colNames[0].LoadStringA(IDS_Vis);
	for (int i=1; i<6; i++)
		colNames[i].LoadStringA(IDS_NetCombineCols+i-1);
	m_list_ctrl.InsertColumn( COL_VIS, colNames[0], LVCFMT_LEFT, 25 );
	m_list_ctrl.InsertColumn( COL_NAME, colNames[1], LVCFMT_LEFT, 140 );
	m_list_ctrl.InsertColumn( COL_PINS, colNames[2], LVCFMT_LEFT, 40 );
	m_list_ctrl.InsertColumn( COL_WIDTH, colNames[3], LVCFMT_LEFT, 40 );
	m_list_ctrl.InsertColumn( COL_VIA_W, colNames[4], LVCFMT_LEFT, 40 );   
	m_list_ctrl.InsertColumn( COL_HOLE_W, colNames[5], LVCFMT_LEFT, 40 );

	int iItem = 0;
	for( int i=0; i<nli.GetSize(); i++ )
	{
		if( nli[i].deleted == FALSE )
		{
			nItem = m_list_ctrl.InsertItem( iItem, "" );
			m_list_ctrl.SetItemData( iItem, (LPARAM)i );
			m_list_ctrl.SetItem( iItem, COL_NAME, LVIF_TEXT, nli[i].name, 0, 0, 0, 0 );
			str.Format( "%d", nli[i].ref_des.GetSize() );
			m_list_ctrl.SetItem( iItem, COL_PINS, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", nli[i].w/NM_PER_MIL );
			m_list_ctrl.SetItem( iItem, COL_WIDTH, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", nli[i].v_w/NM_PER_MIL );
			m_list_ctrl.SetItem( iItem, COL_VIA_W, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", nli[i].v_h_w/NM_PER_MIL );
			m_list_ctrl.SetItem( iItem, COL_HOLE_W, LVIF_TEXT, str, 0, 0, 0, 0 );
			ListView_SetCheckState( m_list_ctrl, nItem, nli[i].visible );
		}
	}
	m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
}

void CDlgNetlist::OnLvnColumnclickListNet(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int column = pNMLV->iSubItem;
	if( column == COL_NAME )
	{
		if( m_sort_type == SORT_UP_NAME )
			m_sort_type = SORT_DOWN_NAME;
		else
			m_sort_type = SORT_UP_NAME;
		m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
	}
	else if( column == COL_WIDTH )
	{
		if( m_sort_type == SORT_UP_WIDTH )
			m_sort_type = SORT_DOWN_WIDTH;
		else
			m_sort_type = SORT_UP_WIDTH;
		m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
	}
	else if( column == COL_VIA_W )
	{
		if( m_sort_type == SORT_UP_VIA_W )
			m_sort_type = SORT_DOWN_VIA_W;
		else
			m_sort_type = SORT_UP_VIA_W;
		m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
	}
	else if( column == COL_HOLE_W )
	{
		if( m_sort_type == SORT_UP_HOLE_W )
			m_sort_type = SORT_DOWN_HOLE_W;
		else
			m_sort_type = SORT_UP_HOLE_W;
		m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
	}
	else if( column == COL_PINS )
	{
		if( m_sort_type == SORT_UP_PINS )
			m_sort_type = SORT_DOWN_PINS;
		else
			m_sort_type = SORT_UP_PINS;
		m_list_ctrl.SortItems( ::CompareNetlist, m_sort_type );
	}
	*pResult = 0;
}


void CDlgNetlist::OnBnClickedButtonVisible()
{
	// CPT2 changed the logic slightly 
	bool bAllSelected = true;
	for( int i=0; i<m_list_ctrl.GetItemCount(); i++ )
		if (!ListView_GetCheckState(m_list_ctrl, i))
			{ bAllSelected = false; break; }
	int newState = !bAllSelected;
	for( int i=0; i<m_list_ctrl.GetItemCount(); i++ )
		ListView_SetCheckState( m_list_ctrl, i, newState );
	for( int i=0; i<nli.GetSize(); i++ )
		nli[i].visible = newState;
}

void CDlgNetlist::OnBnClickedButtonEdit()
{
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 ) {
		CString s ((LPCSTR) IDS_YouHaveNoNetSelected);
		AfxMessageBox( s );
	}
	else if( n_sel > 1 ) {
		CString s ((LPCSTR) IDS_YouHaveMoreThanOneNetSelected);
		AfxMessageBox( s );
	}
	else
	{
		POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
		if (pos == NULL)
			ASSERT(0);
		int nItem = m_list_ctrl.GetNextSelectedItem(pos);
		int i = m_list_ctrl.GetItemData( nItem );

		// prepare and invoke dialog
		CFreePcbView * view = theApp.m_view;
		CFreePcbDoc * doc = theApp.m_doc;
		CDlgEditNet dlg;
		dlg.Initialize( &nli, i, m_plist, FALSE, ListView_GetCheckState( m_list_ctrl, nItem ),
						MIL, &(doc->m_w), &(doc->m_v_w), &(doc->m_v_h_w) );
		int ret = dlg.DoModal();
		if( ret == IDOK )
		{
			// implement edits into nli and update m_list_ctrl
			DrawListCtrl();
			OnNMClickListNet(NULL, NULL);			// CPT2, to ensure appropriate buttons are disabled.
		}
	}
}

void CDlgNetlist::OnBnClickedButtonAdd()
{
	// prepare CDlgEditNet
	CFreePcbView * view = theApp.m_view;
	CFreePcbDoc * doc = theApp.m_doc;
	CDlgEditNet dlg;
	dlg.Initialize( &nli, -1, m_plist, TRUE, TRUE,
						MIL, &doc->m_w, &doc->m_v_w, &doc->m_v_h_w );
	// invoke dialog
	int ret = dlg.DoModal();
	if( ret == IDOK )
		// net added, update m_list_ctrl
		DrawListCtrl();
}

void CDlgNetlist::OnBnClickedOk()
{
	OnOK();
}

void CDlgNetlist::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

void CDlgNetlist::OnBnClickedButtonSelectAll()
{
	for( int i=0; i<nli.GetSize(); i++ )
		m_list_ctrl.SetItemState( i, LVIS_SELECTED, LVIS_SELECTED );
	OnNMClickListNet( NULL, NULL );
}

void CDlgNetlist::OnBnClickedButtonDelete()
{
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
	{
		CString s ((LPCSTR) IDS_YouHaveNoNetSelected);
		AfxMessageBox( s );
	}
	else
	{
		// now delete them
		while( m_list_ctrl.GetSelectedCount() )
		{
			POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
			int i_sel = m_list_ctrl.GetNextSelectedItem( pos );
			int i = m_list_ctrl.GetItemData( i_sel );
			nli[i].deleted = TRUE;
			m_list_ctrl.DeleteItem( i_sel );
		}
		OnNMClickListNet(NULL, NULL);			// CPT2, to ensure appropriate buttons are disabled.
	}
}

void CDlgNetlist::OnBnClickedButtonNLWidth()
{
	CString str;
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
	{
		CString s ((LPCSTR) IDS_YouHaveNoNetSelected);
		AfxMessageBox( s );
		return;
	}
	CFreePcbView * view = theApp.m_view; 
	CFreePcbDoc * doc = theApp.m_doc;
	CDlgSetTraceWidths dlg;
	dlg.m_w = &doc->m_w;
	dlg.m_v_w = &doc->m_v_w;
	dlg.m_v_h_w = &doc->m_v_h_w;
	dlg.m_width = 0;
	dlg.m_via_width = 0;
	dlg.m_hole_width = 0;
	if( n_sel == 1 )
	{
		POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
		int iItem = m_list_ctrl.GetNextSelectedItem( pos );
		int i = m_list_ctrl.GetItemData( iItem );
		dlg.m_width = nli[i].w;
		dlg.m_via_width = nli[i].v_w;
		dlg.m_hole_width = nli[i].v_h_w;
	}
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
		while( pos )
		{
			int iItem = m_list_ctrl.GetNextSelectedItem( pos );
			int i = m_list_ctrl.GetItemData( iItem );
			if( dlg.m_width != -1 )
				nli[i].w = dlg.m_width;
			if( dlg.m_via_width != -1 )
			{
				nli[i].v_w = dlg.m_via_width;
				nli[i].v_h_w = dlg.m_hole_width;
			}
			nli[i].apply_trace_width = dlg.m_apply_trace;
			nli[i].apply_via_width = dlg.m_apply_via;
			str.Format( "%d", nli[i].w/NM_PER_MIL );
			m_list_ctrl.SetItem( iItem, COL_WIDTH, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", nli[i].v_w/NM_PER_MIL );
			m_list_ctrl.SetItem( iItem, COL_VIA_W, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", nli[i].v_h_w/NM_PER_MIL );
			m_list_ctrl.SetItem( iItem, COL_HOLE_W, LVIF_TEXT, str, 0, 0, 0, 0 );
		}
	}
}

void CDlgNetlist::OnBnClickedButtonCombine()
{
	// CPT2 new.
	CString str;
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel < 2 )
		return;						// Weird --- button should only be enabled when 2+ items are selected
	CArray<CString> names;			// Names of the selected items
	CArray<int> pins;				// Numbers of pins for the selected items
	POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
	while( pos )
	{
		int iItem = m_list_ctrl.GetNextSelectedItem( pos );
		int i = m_list_ctrl.GetItemData( iItem );
		names.Add(nli[i].name);
		pins.Add(nli[i].pin_name.GetSize());
	}
	CDlgNetCombine dlg;
	dlg.Initialize( &names, &pins );
	int ret = dlg.DoModal();
	if( ret != IDOK )
		return;
	
	CString newName = dlg.m_new_name;
	// Determine which net-list-info item corresponds to new_name.
	int iNew = -1;
	pos = m_list_ctrl.GetFirstSelectedItemPosition();
	while( pos )
	{
		int iItem = m_list_ctrl.GetNextSelectedItem( pos );
		int i = m_list_ctrl.GetItemData( iItem );
		if (nli[i].name == newName)
			{ iNew = i; break; }
	}
	ASSERT( iNew>=0 );
	// Now mark the "merge_into" members of the other selected nets, and reassign their pins to net #iNew.
	pos = m_list_ctrl.GetFirstSelectedItemPosition();
	while( pos )
	{
		int iItem = m_list_ctrl.GetNextSelectedItem( pos );
		int i = m_list_ctrl.GetItemData( iItem );
		if (i==iNew) 
			{ nli[i].modified = true; continue; }
		nli[i].merge_into = iNew;
		nli[i].deleted = true;
		for (int j = 0; j < nli[i].pin_name.GetSize(); j++)
			nli[iNew].ref_des.Add( nli[i].ref_des[j] ),
			nli[iNew].pin_name.Add( nli[i].pin_name[j] );
		nli[i].ref_des.RemoveAll();
		nli[i].pin_name.RemoveAll();
	}
	DrawListCtrl();
	OnNMClickListNet(NULL, NULL);			// CPT2, to ensure appropriate buttons are disabled.
}

void CDlgNetlist::OnBnClickedDeleteNetsWithNoPins()
{
	for( int iItem=m_list_ctrl.GetItemCount()-1; iItem>=0; iItem-- )
	{
		int i_net = m_list_ctrl.GetItemData( iItem );
		if( nli[i_net].ref_des.GetSize() == 0 )
		{
			nli[i_net].deleted = TRUE;
			m_list_ctrl.DeleteItem( iItem );
		}
	}
}



void CDlgNetlist::OnNMClickListNet(NMHDR *pNMHDR, LRESULT *pResult)
{
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
	{
		m_button_edit.EnableWindow(FALSE);
		m_button_delete_single.EnableWindow(FALSE);
		m_button_nl_width.EnableWindow(FALSE);
		m_button_delete.EnableWindow(FALSE);
		m_button_combine.EnableWindow(FALSE);
	}
	else if( n_sel == 1 )
	{
		m_button_edit.EnableWindow(TRUE);
		m_button_delete_single.EnableWindow(TRUE);
		m_button_nl_width.EnableWindow(FALSE);
		m_button_delete.EnableWindow(FALSE);
		m_button_combine.EnableWindow(FALSE);
	}
	else
	{
		m_button_edit.EnableWindow(FALSE);
		m_button_delete_single.EnableWindow(FALSE);
		m_button_nl_width.EnableWindow(TRUE);
		m_button_delete.EnableWindow(TRUE);
		m_button_combine.EnableWindow(TRUE);
	}
	if( pResult )
		*pResult = 0;
}

