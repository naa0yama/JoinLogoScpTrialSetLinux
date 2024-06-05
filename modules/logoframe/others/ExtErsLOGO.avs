#
# 開始・終了時の片フィールドロゴに対応したEraseLOGO
#   start endは片フィールドロゴも含めたロゴの開始終了フレームを入力
#   itype_s itype_e はそれぞれ開始・終了フレームでのロゴ表示箇所を示す
#     （表示両フィールド(0)、TOPフィールド(1)、BOTTOMフィールド(2)）
# 書き方例
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

    #通常ロゴ消し
    exstart = (itype_s == 0)? start : start+1
    exend   = (itype_e == 0)? end   : end-1
    clip =clip.EraseLOGO( logofile=logofile, start=exstart, end=exend,
\                         fadein=fadein, fadeout=fadeout,
\                         interlaced=true)
    #片フィールドロゴ消し
    # 最終フレーム番号の取得
    final_frame = clip.FrameCount() - 1
    #開始地点片フィールド　ロゴ消し
    framenumber = (itype_s > 0) ? start : nop()
    is_topfield = (itype_s == 1)? true : false
    Sfr = (itype_s > 0) ? clip.HalfFieldDeLogo(framenumber,topfieldlogo = is_topfield, logofile=logofile) : nop()
    clip = (itype_s > 0) ? Trim(clip,0,framenumber-1) ++ Trim(Sfr,0,0) ++ Trim(clip,framenumber+1,final_frame) : clip

    #終了地点片フィールド　ロゴ消し
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

