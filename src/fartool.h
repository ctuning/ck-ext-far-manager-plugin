#ifndef __FARTOOL_H
#define __FARTOOL_H

struct TBlockInfo
{
    int Type,    
        StartRow,
        EndRow,
        StartCol,
        EndCol;
};

struct TEditorPos
{
    TEditorPos() { Default();};

    int Row,Col;
    int TopRow,LeftCol;

    void Default()
    {
        Row=-1; Col=-1; TopRow=-1; LeftCol=-1;
    }
};

class TFarEditor 
{
    public:

    TFarEditor(PluginStartupInfo *Info);
    ~TFarEditor();

    private:
    PluginStartupInfo FarInfo;

    public:
    TEditorPos GetPos();
    void       SetPos(TEditorPos pos);
    void       SetPos(int line,int col,int topline=-1, int leftcol=-1);

    void GetString(EditorGetString *gs,int lineno=-1);
    void SetString(char *src,int lineno=-1);

    void InsertText(char *text)
        { FarInfo.EditorControl(ECTL_INSERTTEXT,text);};

    void ProcessKey(char ascii);
    void ProcessVKey(WORD vkey);
    void ProcessKey(INPUT_RECORD *r);

    void Redraw()
        { FarInfo.EditorControl(ECTL_REDRAW,0);};

    void GetInfo();

    void GetBlockInfo();

    int  Id;
    int  TabSize;

    TBlockInfo Block;

};

#endif

