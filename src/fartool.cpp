#include <windows.h>

#include <string.h>

#include "plugin.hpp"
#include "fartool.h"

#ifdef DEBUG
#define D(i) i
#include "log.h"
#else
#define D(i)
#endif

TFarEditor::TFarEditor(PluginStartupInfo *ei)
{
    FarInfo=*ei;
}

TFarEditor::~TFarEditor()
{
    //reserved for future
}

TEditorPos TFarEditor::GetPos()
{
    TEditorPos r;
    EditorInfo ei;

    FarInfo.EditorControl(ECTL_GETINFO,&ei);

    r.Row=ei.CurLine;
    r.Col=ei.CurPos;
    r.TopRow=ei.TopScreenLine;
    r.LeftCol=ei.LeftPos;

    return r;
}

void TFarEditor::SetPos(TEditorPos pos)
{
    EditorSetPosition sp;

    sp.CurLine=pos.Row;
    sp.CurPos=pos.Col;
    sp.TopScreenLine=pos.TopRow;
    sp.LeftPos=pos.LeftCol;
    sp.CurTabPos=-1;
    sp.Overtype=-1;
    FarInfo.EditorControl(ECTL_SETPOSITION,&sp);
}

void TFarEditor::SetPos(int line,int col,int topline, int leftcol)
{
    EditorSetPosition sp;

    sp.CurLine=line;
    sp.CurPos=col;
    sp.TopScreenLine=topline;
    sp.LeftPos=leftcol;
    sp.CurTabPos=-1;
    sp.Overtype=-1;
    FarInfo.EditorControl(ECTL_SETPOSITION,&sp);
}

void TFarEditor::GetString(EditorGetString *gs,int line)
{
    gs->StringNumber=line;
    FarInfo.EditorControl(ECTL_GETSTRING,gs);    
}

void TFarEditor::SetString(char *src,int line)
{
    EditorSetString st;

    st.StringNumber=line;
    st.StringText=src;
    st.StringEOL=0;
    st.StringLength=strlen(src);

    FarInfo.EditorControl(ECTL_SETSTRING, &st);
}

void TFarEditor::ProcessKey(char ascii)
{
    INPUT_RECORD tr;
    tr.EventType=KEY_EVENT;
    tr.Event.KeyEvent.bKeyDown=TRUE;
    tr.Event.KeyEvent.wRepeatCount=1;
    tr.Event.KeyEvent.wVirtualKeyCode=0;
    tr.Event.KeyEvent.uChar.AsciiChar=ascii;
    tr.Event.KeyEvent.dwControlKeyState=0;
    FarInfo.EditorControl(ECTL_PROCESSINPUT,&tr);    
}

void TFarEditor::ProcessVKey(WORD vkey)
{
    INPUT_RECORD tr;
    tr.EventType=KEY_EVENT;
    tr.Event.KeyEvent.bKeyDown=TRUE;
    tr.Event.KeyEvent.wRepeatCount=1;
    tr.Event.KeyEvent.wVirtualKeyCode=vkey;
    tr.Event.KeyEvent.dwControlKeyState=0;
    FarInfo.EditorControl(ECTL_PROCESSINPUT,&tr);
}

void TFarEditor::ProcessKey(INPUT_RECORD *r)
{
    FarInfo.EditorControl(ECTL_PROCESSINPUT,r);
}

void TFarEditor::GetInfo()
{
    EditorInfo ei;

    FarInfo.EditorControl(ECTL_GETINFO,&ei);
    Id=ei.EditorID;
    TabSize=ei.TabSize;
}

void TFarEditor::GetBlockInfo()
{
    EditorInfo ei;
    EditorGetString gs;
    TEditorPos ep;
    int i,pe;

    FarInfo.EditorControl(ECTL_GETINFO,&ei);
    Block.Type=ei.BlockType;
    if (ei.BlockType==BTYPE_NONE)
        return;

    Block.StartRow=ei.BlockStartLine;

    ep=GetPos();

    SetPos(Block.StartRow,-1);
    GetString(&gs);
    Block.StartCol=gs.SelStart;

    i=ei.BlockStartLine;
    pe=0;
    D(SysLog("GetBlockInfo: type=%i",Block.Type));
    while (1)
    {
        SetPos(i,-1);
        GetString(&gs);
        D(SysLog("i=%i, start=%i, end=%i",i,gs.SelStart,gs.SelEnd));
        if (Block.Type==BTYPE_STREAM)
        {
            if ( gs.SelEnd!=-1)
            {
                Block.EndRow=i;
                Block.EndCol=gs.SelEnd;
                break;
            }
        }
        else // BTYPE_COLUNM
        {
            if ( gs.SelStart==-1)
            {
                Block.EndRow=i-1;
                Block.EndCol=pe;
                break;
            }
            pe=gs.SelEnd;
        }
        i++;
        if (i>ei.TotalLines)
            break;
    }
    SetPos(ep);
}
