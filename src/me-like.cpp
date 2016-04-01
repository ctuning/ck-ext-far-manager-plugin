/* 
  Productivity plugin for FAR Manager

 See CK LICENSE.txt for licensing details
 See CK Copyright.txt for copyright details

 Developer: Grigori Fursin
*/


#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <process.h> 

#include "plugin.hpp"
#include "fartool.h"

#ifdef DEBUG
#include "log.h"
#define D(i) i
#include "time.h"
#else
#define D(i)
#endif

static struct PluginStartupInfo Info;
static struct FarStandardFunctions FSF;

#define PROCESS_EVENT 0
#define IGNORE_EVENT  1

struct TMark;
struct TMark
{
    int Id;
    TEditorPos Pos;
    TMark *Next;
};

TFarEditor *Editor;
TMark *Head;

int busy=0;

void PushMark();
void PopMark();
void UnIndentBlock();
void IndentBlock();
void FGGOpenIE();
void FGGOpenFirefox();
void FGGOpenChrome();
void FGGCopyToClipboard();
void FGGCopyToClipboardAfter();
void GoogleSearch();
void FGGcKCall(char *);

int CountSpaces(const char *text);
void AddToStack(int id,TEditorPos ep);
int GetLastMark(int id,TEditorPos &ep);

void WINAPI _export SetStartupInfo(struct PluginStartupInfo *psInfo)
{
    Info=*psInfo;
    Editor=new TFarEditor(psInfo);
    Head=0;

    FSF=*psInfo->FSF;
    Info.FSF=&FSF; // now Info.FSF will point to the correct local address
}

void WINAPI _export ExitFAR()
{
    delete Editor;
    TMark *c,*n;
    c=Head;
    while (c)
    {
        n=c->Next;
        delete c;
        c=n;
    }
}

void WINAPI _export GetPluginInfo(struct PluginInfo *Info)
{
    Info->StructSize=sizeof(*Info);
    Info->Flags=PF_EDITOR|PF_DISABLEPANELS;
    Info->DiskMenuStringsNumber=0;
    Info->PluginMenuStrings=0;
    Info->PluginMenuStringsNumber=0;
    Info->PluginConfigStringsNumber=0;
}

#pragma argsused
HANDLE WINAPI _export OpenPlugin(int OpenFrom,int Item)
{
    return(INVALID_HANDLE_VALUE);
}

int WINAPI _export ProcessEditorInput(INPUT_RECORD *Rec)
{
    EditorGetString gs;
    struct EditorInfo ei;
    TEditorPos epos;
    //char c;
    WORD vkey;
    DWORD ks;
    char *ptr;

    if (busy)
        return PROCESS_EVENT;
    if (Rec->EventType!=KEY_EVENT)
        return PROCESS_EVENT;
    if (Rec->Event.KeyEvent.bKeyDown==0)
        return PROCESS_EVENT;

    //c=Rec->Event.KeyEvent.uChar.AsciiChar;
    vkey=Rec->Event.KeyEvent.wVirtualKeyCode;
    ks=Rec->Event.KeyEvent.dwControlKeyState;

    ks&=~(CAPSLOCK_ON|ENHANCED_KEY|NUMLOCK_ON|SCROLLLOCK_ON);

    if ( vkey==VK_F4 && ks==0 )
        PushMark();    
    else if ( vkey==VK_F4 && (ks & SHIFT_PRESSED) )
        PopMark();
    else if ( vkey==VK_F2 && (ks & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)) )
        UnIndentBlock();
    else if ( vkey==VK_F3 && (ks & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)) )
        IndentBlock();
    else if ( vkey==VK_F6 && (ks & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)) )
        GoogleSearch();
    else if ( vkey==VK_F1 && (ks & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) )
        FGGOpenIE();
    else if ( vkey==VK_F2 && (ks & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) )
        FGGOpenFirefox();
    else if ( vkey==VK_F12 && (ks & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) )
        FGGOpenChrome();
    else if ( vkey==VK_F3 && (ks & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) )
        FGGCopyToClipboard();
    else if ( vkey==VK_F4 && (ks & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) )
        FGGCopyToClipboardAfter();
    else if ( vkey=='0' && (ks & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) )
        FGGcKCall("path");
    else if ( vkey=='1' && (ks & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) )
        FGGcKCall("far");
    else if ( vkey=='2' && (ks & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) )
        FGGcKCall("firefox");
    else if ( vkey=='3' && (ks & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) )
        FGGcKCall("firefox_external");
    else if ( vkey=='4' && (ks & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) )
        FGGcKCall("firefox_external_cm");

    return PROCESS_EVENT;
}

void PushMark()
{
    TEditorPos ep;    
    Editor->GetInfo();
    ep=Editor->GetPos();
    D(SysLog("PushMark: Id=%i, mark=(%i,%i,%i,%i)",
            Editor->Id,ep.Row,ep.Col,ep.TopRow,ep.LeftCol));
    AddToStack(Editor->Id,ep);
}

void PopMark()
{
    TEditorPos ep;    
    int havemark;

    Editor->GetInfo();
    D(SysLog("PopMark: Id=%i",Editor->Id));

    havemark=GetLastMark(Editor->Id,ep);
    if (havemark)
    {
        Editor->SetPos(ep);    
        Editor->Redraw();
    }
}

void UnIndentBlock()
{
    TEditorPos ep;
    EditorGetString gs;
    char *ptr;
    int tabsize;
    int spaces,newspaces,endrow;

    Editor->GetInfo();
    tabsize=Editor->TabSize;

    D(SysLog("UnIndentBlock: Id=%i",Editor->Id));

    ep=Editor->GetPos();

    Editor->GetBlockInfo();
    if (Editor->Block.Type==BTYPE_NONE)
    {
        D(SysLog("no block selected..."));
        return;    
    }
    D(SysLog("current block is '%s' (%i,%i) to (%i,%i)",
            (Editor->Block.Type==BTYPE_STREAM ? "stream" : "vertical"),
            Editor->Block.StartRow,Editor->Block.StartCol,
            Editor->Block.EndRow,Editor->Block.EndCol));

    ptr=new char[4096];

    endrow=Editor->Block.EndRow-(Editor->Block.EndCol==0? 1 : 0);

    for (int i=Editor->Block.StartRow; i<=endrow; i++)
    {
        memset(ptr,0,4096);
        Editor->SetPos(i,-1);
        Editor->GetString(&gs);
        if (gs.StringLength==0)
            continue;
        spaces=CountSpaces(gs.StringText);
        if (spaces==0)
            continue;
//original        newspaces=(int(spaces/tabsize)-1)*tabsize;
//next is fgg change
        newspaces=spaces-1;
        if (newspaces<0)
            newspaces=0;
        for (int j=0; j<newspaces; j++)
            ptr[j]=' ';
        strcpy(ptr+newspaces,gs.StringText+spaces);
        Editor->SetString(ptr);
    }

    Editor->SetPos(ep);
    Editor->Redraw();
    delete ptr;
    return;
}

void IndentBlock()
{
    TEditorPos ep;
    EditorGetString gs;
    char *ptr;
    int tabsize;
    int spaces,newspaces,endrow;

    Editor->GetInfo();
    tabsize=Editor->TabSize;

    D(SysLog("IndentBlock: Id=%i",Editor->Id));

    ep=Editor->GetPos();

    Editor->GetBlockInfo();
    if (Editor->Block.Type==BTYPE_NONE)
    {
        D(SysLog("no block selected..."));
        return;
    }
    D(SysLog("current block is '%s' (%i,%i) to (%i,%i)",
            (Editor->Block.Type==BTYPE_STREAM ? "stream" : "vertical"),
            Editor->Block.StartRow,Editor->Block.StartCol,
            Editor->Block.EndRow,Editor->Block.EndCol));

    ptr=new char[4096];

    endrow=Editor->Block.EndRow-(Editor->Block.EndCol==0? 1 : 0);

    for (int i=Editor->Block.StartRow; i<=endrow; i++)
    {
        memset(ptr,0,4096);
        Editor->SetPos(i,-1);
        Editor->GetString(&gs);
        if (gs.StringLength==0)
            continue;
        spaces=CountSpaces(gs.StringText);
//original        newspaces=(int(spaces/tabsize)+1)*tabsize;
//next is fgg change
        newspaces=spaces+1;
        for (int j=0; j<newspaces; j++)
            ptr[j]=' ';
        strcpy(ptr+newspaces,gs.StringText+spaces);
        Editor->SetString(ptr);
    }

    Editor->SetPos(ep);
    Editor->Redraw();
    delete ptr;
    return;
}

void GoogleSearch()
{
    char *NewString;
    char *NewString1;
    struct EditorInfo ei;
    Info.EditorControl(ECTL_GETINFO,&ei);

    // Nothing selected?
    if (ei.BlockType==BTYPE_NONE) return;

    // Current line number
    int CurLine=ei.CurLine;

    struct EditorGetString egs;

    egs.StringNumber=CurLine;

    // If can't get line
    if (!Info.EditorControl(ECTL_GETSTRING,&egs))
      return; // Exit

    // If whole line (with EOL) is selected
    if (egs.SelEnd==-1 || egs.SelEnd>egs.StringLength)
    {
      egs.SelEnd=egs.StringLength;
      if (egs.SelEnd<egs.SelStart)
        egs.SelEnd=egs.SelStart;
    }

    // Memory allocation
    //NewString=(char *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, egs.StringLength+1);
    //NewString1=(char *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, egs.StringLength+100);
    // If memory couldn't be allocated
    //if(!NewString || !NewString1)
    //   return;

    NewString=new char[4096];
    NewString1=new char[4096];

    int len=egs.SelEnd-egs.SelStart;
    if (len<3900)
    {
      memcpy(NewString,&egs.StringText[egs.SelStart],len);
      NewString[len]=0;

      strcpy(NewString1, "\"http://www.google.com/search?hl=en&q=%22");
      strcat(NewString1, NewString);
      strcat(NewString1, "%22\"");

      ShellExecute(NULL,"open", "firefox", NewString1, "", SW_SHOW);
    }
    
    delete NewString;
    delete NewString1;
}

void FGGOpenIE()
{
    TEditorPos ep;
    EditorGetString gs;
    EditorSelect es;
    char *ptr;
    int i,i1,i2;

    Editor->GetInfo();

    ep=Editor->GetPos();

    ptr=new char[4096];

    Editor->GetString(&gs);
    if (gs.StringLength!=0)
    {
      i1=ep.Col;
      i2=ep.Col;

      while (i1>0)
      {
        if (gs.StringText[i1]==' ') break;
        i1--;
      }
      if (i1!=0) i1++;

      while (i2<strlen(gs.StringText))
      {
        if (gs.StringText[i2]==' ') break;
        i2++;
      }

      for (i=i1; i<i2; i++) ptr[i-i1]=gs.StringText[i];
      ptr[i-i1]=0;

      es.BlockType=BTYPE_STREAM;
      es.BlockStartLine=ep.Row;
      es.BlockStartPos=i1;
      es.BlockWidth=i2-i1;
      es.BlockHeight=1;
      Info.EditorControl(ECTL_SELECT,(void*)&es);

      FSF.CopyToClipboard(FSF.Trim(ptr));

      ShellExecute(NULL,"open", "iexplore",ptr,"",SW_SHOW);
    }

    delete ptr;
    return;
}

void FGGOpenFirefox()
{
    TEditorPos ep;
    EditorGetString gs;
    EditorSelect es;
    char *ptr;
    int i,i1,i2;

    Editor->GetInfo();

    ep=Editor->GetPos();

    ptr=new char[4096];

    Editor->GetString(&gs);
    if (gs.StringLength!=0)
    {
      i1=ep.Col;
      i2=ep.Col;

      while (i1>0)
      {
        if (gs.StringText[i1]==' ') break;
        i1--;
      }
      if (i1!=0) i1++;

      while (i2<strlen(gs.StringText))
      {
        if (gs.StringText[i2]==' ') break;
        i2++;
      }

      for (i=i1; i<i2; i++) ptr[i-i1]=gs.StringText[i];
      ptr[i-i1]=0;

      es.BlockType=BTYPE_STREAM;
      es.BlockStartLine=ep.Row;
      es.BlockStartPos=i1;
      es.BlockWidth=i2-i1;
      es.BlockHeight=1;
      Info.EditorControl(ECTL_SELECT,(void*)&es);

      FSF.CopyToClipboard(FSF.Trim(ptr));

      ShellExecute(NULL,"open", "firefox",ptr,"",SW_SHOW);
    }

    delete ptr;
    return;
}

void FGGOpenChrome()
{
    TEditorPos ep;
    EditorGetString gs;
    EditorSelect es;
    char *ptr;
    int i,i1,i2;

    Editor->GetInfo();

    ep=Editor->GetPos();

    ptr=new char[4096];

    Editor->GetString(&gs);
    if (gs.StringLength!=0)
    {
      i1=ep.Col;
      i2=ep.Col;

      while (i1>0)
      {
        if (gs.StringText[i1]==' ') break;
        i1--;
      }
      if (i1!=0) i1++;

      while (i2<strlen(gs.StringText))
      {
        if (gs.StringText[i2]==' ') break;
        i2++;
      }

      for (i=i1; i<i2; i++) ptr[i-i1]=gs.StringText[i];
      ptr[i-i1]=0;

      es.BlockType=BTYPE_STREAM;
      es.BlockStartLine=ep.Row;
      es.BlockStartPos=i1;
      es.BlockWidth=i2-i1;
      es.BlockHeight=1;
      Info.EditorControl(ECTL_SELECT,(void*)&es);

      FSF.CopyToClipboard(FSF.Trim(ptr));

      ShellExecute(NULL, "open", "chrome", ptr, "", SW_SHOW);
    }

    delete ptr;
    return;
}

void FGGCopyToClipboard()
{
    TEditorPos ep;
    EditorGetString gs;
    EditorSelect es;
    char *ptr;
    int i,i1,i2;

    Editor->GetInfo();

    ep=Editor->GetPos();

    ptr=new char[4096];

    Editor->GetString(&gs);
    if (gs.StringLength!=0)
    {
      i1=ep.Col;
      i2=ep.Col;

      while (i1>0)
      {
        if (gs.StringText[i1]==' ' || gs.StringText[i1]==':') break;
        i1--;
      }
      if (i1!=0) i1++;

      while (i2<strlen(gs.StringText))
      {
        if (gs.StringText[i2]==' ' || gs.StringText[i2]==':') break;
        i2++;
      }

      for (i=i1; i<i2; i++) ptr[i-i1]=gs.StringText[i];
      ptr[i-i1]=0;

      es.BlockType=BTYPE_STREAM;
      es.BlockStartLine=ep.Row;
      es.BlockStartPos=i1;
      es.BlockWidth=i2-i1;
      es.BlockHeight=1;
      Info.EditorControl(ECTL_SELECT,(void*)&es);

      FSF.CopyToClipboard(FSF.Trim(ptr));
    }

    delete ptr;
    return;
}

void FGGCopyToClipboardAfter()
{
    TEditorPos ep;
    EditorGetString gs;
    EditorSelect es;
    char *ptr;
    int i,i1,i2;

    Editor->GetInfo();

    ep=Editor->GetPos();

    ptr=new char[4096];

    Editor->GetString(&gs);
    if (gs.StringLength!=0)
    {
      i1=ep.Col;
      i2=ep.Col;

      while (i1<strlen(gs.StringText))
      {
        if (gs.StringText[i1]==' ' || gs.StringText[i1]==':') break;
        i1++;
      }
      i1++;

      i2=i1;
      while (i2<strlen(gs.StringText))
      {
        if (gs.StringText[i2]==' ' || gs.StringText[i2]==':') break;
        i2++;
      }

      for (i=i1; i<i2; i++) ptr[i-i1]=gs.StringText[i];
      ptr[i-i1]=0;

      es.BlockType=BTYPE_STREAM;
      es.BlockStartLine=ep.Row;
      es.BlockStartPos=i1;
      es.BlockWidth=i2-i1;
      es.BlockHeight=1;
      Info.EditorControl(ECTL_SELECT,(void*)&es);

      FSF.CopyToClipboard(FSF.Trim(ptr));
    }

    delete ptr;
    return;
}

void AddToStack(int id,TEditorPos ep)
{
    TMark *m,*c;

    m=new TMark;

    m->Id=id; m->Pos=ep;
    
    m->Next=Head;
    Head=m;
}

int GetLastMark(int id,TEditorPos &ep)
{
    TMark *c,*p;

    c=Head;
    p=0;
    while (c)
    {
        if (c->Id==id)
        {
            ep=c->Pos;
            if (p)
                p->Next=c->Next;
            else
                Head=c->Next;                
            delete c;
            return 1;
        }
        p=c;
        c=c->Next;
    }
    return 0;

}

int CountSpaces(const char *text)
{
    int i=0;
    while (1)
    {
        if (text[i]==' ')
            i++;
        else
            break;
    }
    return i;
}

void FGGcKCall(char *param)
{
    #define imax 1024
    TEditorPos ep;
    EditorGetString gs;
    EditorSelect es;
    char *ptr;
    int i,i1,i2;
    char cmd[imax];
    char buf[imax];
    char out[imax];
    char ext[imax];

    Editor->GetInfo();

    ep=Editor->GetPos();

    ptr=new char[4096];

    strcpy(ext,"");

    Editor->GetString(&gs);
    if (gs.StringLength!=0)
    {
      i1=ep.Col;
      i2=ep.Col;

      while (i1>0)
      {
        if (gs.StringText[i1]==' ' || gs.StringText[i1]=='=' || 
            gs.StringText[i1]=='(' || gs.StringText[i1]=='[' ||
            gs.StringText[i1]=='"') break;
        i1--;
      }

      if (gs.StringText[i1]!='"')
      {
        if (i1!=0) i1++;

        while (i2<strlen(gs.StringText))
        {
          if (gs.StringText[i2]==' ' || gs.StringText[i2]=='=' || 
              gs.StringText[i2]=='(' || gs.StringText[i2]=='[' ||
              gs.StringText[i2]==')' || gs.StringText[i2]==']' ||
              gs.StringText[i2]=='"') break;
          i2++;
        }
      }
      else
      {
        while (i2<strlen(gs.StringText))
        {
          if (gs.StringText[i2]=='"') break;
          i2++;
        }
        i2++;
      }

      for (i=i1; i<i2; i++) ptr[i-i1]=gs.StringText[i];
      ptr[i-i1]=0;

      es.BlockType=BTYPE_STREAM;
      es.BlockStartLine=ep.Row;
      es.BlockStartPos=i1;
      es.BlockWidth=i2-i1;
      es.BlockHeight=1;
      Info.EditorControl(ECTL_SELECT,(void*)&es);

      FSF.CopyToClipboard(FSF.Trim(ptr));

      if (strcmp(param, "path")==0)
      {
         sprintf(cmd, "start far.exe /d %s", ptr);
         system(cmd);
      }
      else
      {
         // Check dir extension in CK entry
         i2=0;
         while (i2<strlen(ptr))
         {
           if (ptr[i2]=='/' || ptr[i2]=='\\')
           {
              i2+=1;
              if (i2<strlen(ptr)) 
              {
                strcpy(ext, &ptr[i2]);
                ptr[i2-1]=0;
                break;
              }
           }
           i2++;
         }

         if (strcmp(param, "far")==0)
         {
            sprintf(cmd, "ck find %s", ptr);

            FILE* pipe = _popen(cmd, "r");
            if (pipe)
            {
              strcpy(out,"");
              while(!feof(pipe)) 
              {
                if (fgets(buf, imax-2, pipe) != NULL)
                   strcat(out, buf);
              }
              _pclose(pipe);
            }

            i2=strlen(out);
            if ((i2>0) && (out[i2-1]=='\r' || out[i2-1]=='\n')) out[i2-1]=0;

            i2=strlen(out);
            if ((i2>0) && (out[i2-1]=='\r' || out[i2-1]=='\n')) out[i2-1]=0;

            if (strlen(ext)>0)
            {
               strcat(out, "\\");
               strcat(out, ext);
            }

            sprintf(cmd, "start far.exe /d \"%s\"", out);
            system(cmd);
         }
         else if (strcmp(param, "firefox")==0)
         {
            sprintf(cmd, "\"http://localhost:3344/json?action=load&cid=%s\"", ptr);
            ShellExecute(NULL,"open", "firefox", cmd, "", SW_SHOW);
         }
         else if (strcmp(param, "firefox_external")==0)
         {
            sprintf(cmd, "\"http://cknowledge.org/repo/json.php?action=load&cid=%s\"", ptr);
            ShellExecute(NULL,"open", "firefox", cmd, "", SW_SHOW);
         }
         else if (strcmp(param, "firefox_external_cm")==0)
         {
            sprintf(cmd, "\"http://c-mind.org/repo/?cm_menu=browse&cm_subaction_view&browse_cid=%s\"", ptr);
            ShellExecute(NULL,"open", "firefox", cmd, "", SW_SHOW);
         }
      }
    }

    delete ptr;
    return;
}
