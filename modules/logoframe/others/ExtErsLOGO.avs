#
# �J�n�E�I�����̕Ѓt�B�[���h���S�ɑΉ�����EraseLOGO
#   start end�͕Ѓt�B�[���h���S���܂߂����S�̊J�n�I���t���[�������
#   itype_s itype_e �͂��ꂼ��J�n�E�I���t���[���ł̃��S�\���ӏ�������
#     �i�\�����t�B�[���h(0)�ATOP�t�B�[���h(1)�ABOTTOM�t�B�[���h(2)�j
# ��������
#ExtErsLOGO( start=526, end=6817, itype_s=2, itype_e=1, logofile=Lfile )
#ExtErsLOGO( logofile=Lfile, 526, 6817, 2, 1, 0, 0 )
#
function ExtErsLOGO( clip clip, string "logofile",
\                    int "start", int "end",
\                    int "itype_s", int "itype_e",
\                    int "fadein", int "fadeout"
\                  ){

    fadein  = default(fadein,0)
    fadeout = default(fadeout,0)
    itype_s = default(itype_s,0)
    itype_e = default(itype_e,0)

    #�ʏ탍�S����
    exstart = (itype_s == 0)? start : start+1
    exend   = (itype_e == 0)? end   : end-1
    clip =clip.EraseLOGO( logofile=logofile, start=exstart, end=exend,
\                         fadein=fadein, fadeout=fadeout,
\                         interlaced=true)
    #�Ѓt�B�[���h���S����
    # �ŏI�t���[���ԍ��̎擾
    final_frame = clip.FrameCount() - 1
    #�J�n�n�_�Ѓt�B�[���h�@���S����
    framenumber = (itype_s > 0) ? start : nop()
    is_topfield = (itype_s == 1)? true : false
    Sfr = (itype_s > 0) ? clip.HalfFieldDeLogo(framenumber,topfieldlogo = is_topfield, logofile=logofile) : nop()
    clip = (itype_s > 0) ? Trim(clip,0,framenumber-1) ++ Trim(Sfr,0,0) ++ Trim(clip,framenumber+1,final_frame) : clip

    #�I���n�_�Ѓt�B�[���h�@���S����
    framenumber = (itype_e > 0) ? end : nop()
    is_topfield = (itype_e == 1)? true : false
    Sfr = (itype_e > 0) ? clip.HalfFieldDeLogo(framenumber,topfieldlogo = is_topfield, logofile=logofile) : nop()
    clip = (itype_e > 0) ? Trim(clip,0,framenumber-1) ++ Trim(Sfr,0,0) ++ Trim(clip,framenumber+1,final_frame) : clip

    return clip
}


function HalfFieldDeLogo(clip clp, int "framenumber", bool "topfieldlogo", string "logofile"){
c=clp.Trim(framenumber,framenumber)
d=c.EraseLOGO(logofile=logofile,interlaced=true)
dc = d.SeparateFields() + c.SeparateFields()
cd = c.SeparateFields() + d.SeparateFields()
weaved = (topfieldlogo == GetParity(clp)) ? dc.SelectEvery(4,0,3).Weave() : cd.SelectEvery(4,0,3).Weave()
return weaved.AssumeFPS(Framerate(clp))
}
