/* ---------------------------------------------------------------------------
 *                            Decompression
 * --------------------------------------------------------------------------- */
static unsigned char *res__barrier;
static unsigned char *res__barrier2;
static unsigned char *res__barrier3;
static unsigned char *res__barrier4;
static unsigned char *res__dout;

static unsigned
res__decompress_len(unsigned char *input) {
    return (unsigned int)((input[8] << 24) +
            (input[9] << 16) + (input[10] << 8) + input[11]);
}
static void
res__match(unsigned char *data, unsigned int length) {
  /* INVERSE of memmove... write each byte before copying the next...*/
  assert(res__dout + length <= res__barrier);
  if (res__dout + length > res__barrier) {
    res__dout += length; return;
  }
  if (data < res__barrier4) {
    res__dout = res__barrier+1;
    return;
  }
  while (length--) {
    *res__dout++ = *data++;
  }
}
static void
res__lit(unsigned char *data, unsigned int length) {
  assert(res__dout + length <= res__barrier);
  if (res__dout + length > res__barrier) {
    res__dout += length; return;
  }
  if (data < res__barrier2) {
    res__dout = res__barrier+1;
    return;
  }
  memcpy(res__dout, data, length);
  res__dout += length;
}
static unsigned char*
res__decompress_token(unsigned char *i) {
  #define res__in2(x)   ((i[x] << 8) + i[(x)+1])
  #define res__in3(x)   ((i[x] << 16) + res__in2((x)+1))
  #define res__in4(x)   ((i[x] << 24) + res__in3((x)+1))
  if (*i >= 0x20) {       /* use fewer if's for cases that expand small */
    if (*i >= 0x80)       res__match(res__dout-i[1]-1, (unsigned int)i[0] - 0x80 + 1), i += 2;
    else if (*i >= 0x40)  res__match(res__dout-(res__in2(0) - 0x4000 + 1), (unsigned int)i[2]+1), i += 3;
    else /* *i >= 0x20 */ res__lit(i+1, (unsigned int)i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
  } else { /* more ifs for cases that expand large, since overhead is amortized */
    if (*i >= 0x18)       res__match(res__dout-(unsigned int)(res__in3(0) - 0x180000 + 1), (unsigned int)i[3]+1), i += 4;
    else if (*i >= 0x10)  res__match(res__dout-(unsigned int)(res__in3(0) - 0x100000 + 1), (unsigned int)res__in2(3)+1), i += 5;
    else if (*i >= 0x08)  res__lit(i+2, (unsigned int)res__in2(0) - 0x0800 + 1), i += 2 + (res__in2(0) - 0x0800 + 1);
    else if (*i == 0x07)  res__lit(i+3, (unsigned int)res__in2(1) + 1), i += 3 + (res__in2(1) + 1);
    else if (*i == 0x06)  res__match(res__dout-(unsigned int)(res__in3(1)+1), i[4]+1u), i += 5;
    else if (*i == 0x04)  res__match(res__dout-(unsigned int)(res__in3(1)+1), (unsigned int)res__in2(4)+1u), i += 6;
  }
  return i;
}
static unsigned
res__adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen) {
  const unsigned long ADLER_MOD = 65521;
  unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
  unsigned long i, blocklen = buflen % 5552;
  while (buflen) {
    for (i=0; i + 7 < blocklen; i += 8) {
      s1 += buffer[0]; s2 += s1;
      s1 += buffer[1]; s2 += s1;
      s1 += buffer[2]; s2 += s1;
      s1 += buffer[3]; s2 += s1;
      s1 += buffer[4]; s2 += s1;
      s1 += buffer[5]; s2 += s1;
      s1 += buffer[6]; s2 += s1;
      s1 += buffer[7]; s2 += s1;
      buffer += 8;
    }
    for (; i < blocklen; ++i) {
      s1 += *buffer++; s2 += s1;
    }
    s1 %= ADLER_MOD; s2 %= ADLER_MOD;
    buflen -= (unsigned int)blocklen;
    blocklen = 5552;
  }
  return (unsigned int)(s2 << 16) + (unsigned int)s1;
}
static unsigned
res__decompress(unsigned char *output, unsigned char *i, unsigned int length) {
  unsigned int olen;
  if (res__in4(0) != 0x57bC0000) return 0;
  if (res__in4(4) != 0)          return 0; /* error! stream is > 4GB */
  olen = res__decompress_len(i);

  res__barrier2 = i;
  res__barrier3 = i+length;
  res__barrier = output + olen;
  res__barrier4 = output;
  i += 16;

  res__dout = output;
  for (;;) {
    unsigned char *old_i = i;
    i = res__decompress_token(i);
    if (i == old_i) {
      if (*i == 0x05 && i[1] == 0xfa) {
        assert(res__dout == output + olen);
        if (res__dout != output + olen) return 0;
        if (res__adler32(1, output, olen) != (unsigned) res__in4(2))
          return 0;
        return olen;
      } else {
        assert(0); /* NOTREACHED */
        return 0;
      }
    }
    assert(res__dout <= output + olen);
    if (res__dout > output + olen)
      return 0;
  }
}
static unsigned
res__decode_85_byte(char c) {
  return (unsigned int)((c >= '\\') ? c-36 : c-35);
}
static void
res__decode_85(unsigned char* dst, const unsigned char* src) {
  while (*src) {
    unsigned int tmp =
         1 * res__decode_85_byte((char)src[0]) +
        85 * (res__decode_85_byte((char)src[1]) +
        85 * (res__decode_85_byte((char)src[2]) +
        85 * (res__decode_85_byte((char)src[3]) +
        85 * res__decode_85_byte((char)src[4]))));

    /* we can't assume little-endianess. */
    dst[0] = (unsigned char)((tmp >> 0) & 0xFF);
    dst[1] = (unsigned char)((tmp >> 8) & 0xFF);
    dst[2] = (unsigned char)((tmp >> 16) & 0xFF);
    dst[3] = (unsigned char)((tmp >> 24) & 0xFF);

    src += 5;
    dst += 4;
  }
}
/* clang-format on */

static void *
res_unpack(int *data_siz, const char *src, struct sys *s,
           struct arena *a, struct arena *tmp) {
  unsigned char *data = 0;
  {
    const int com_size = (((int)strlen(src) + 4) / 5) * 4;
    unsigned char *com_buf = arena_alloc(tmp, s, com_size);
    res__decode_85(com_buf, cast(const unsigned char *, src));
    {
      unsigned un_siz = res__decompress_len(com_buf);
      data = arena_alloc(a, s, cast(int, un_siz));
      res__decompress(data, com_buf, un_siz);
      *data_siz = cast(int, un_siz);
    }
  }
  return data;
}
/* ---------------------------------------------------------------------------
 *                                  Font
 * ---------------------------------------------------------------------------
 */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverlength-strings"
#endif

// clang-format off
static const char res__default_fnt[] =
  "7])#######hV0qs'/###[),##/l:$#Q6>##5[n42>c-TH`->>#/e>11NNV=Bv(*:.F?uu#(gRU.o0XGH`$vhLG1hxt9?W`#,5LsCp#-i>.r$<$6pD>Lb';9Crc6tgXmKVeU2cD4Eo3R/"
  "2*>]b(MC;$jPfY.;h^`IWM9<Lh2TlS+f-s$o6Q<BWH`YiU.xfLq$N;$0iR/GX:U(jcW2p/W*q?-qmnUCI;jHSAiFWM.R*kU@C=GH?a9wp8f$e.-4^Qg1)Q-GL(lf(r/7GrRgwV%MS=C#"
  "`8ND>Qo#t'X#(v#Y9w0#1D$CIf;W'#pWUPXOuxXuU(H9M(1<q-UE31#^-V'8IRUo7Qf./L>=Ke$$'5F%)]0^#0X@U.a<r:QLtFsLcL6##lOj)#.Y5<-R&KgLwqJfLgN&;Q?gI^#DY2uL"
  "i@^rMl9t=cWq6##weg>$FBjVQTSDgEKnIS7EM9>ZY9w0#L;>>#Mx&4Mvt//L[MkA#W@lK.N'[0#7RL_&#w+F%HtG9M#XL`N&.,GM4Pg;-<nLENhvx>-VsM.M0rJfLH2eTM`*oJMHRC`N"
  "kfimM2J,W-jXS:)r0wK#@Fge$U>`w'N7G#$#fB#$E^$#:9:hk+eOe--6x)F7*E%?76%^GMHePW-Z5l'&GiF#$956:rS?dA#fiK:)Yr+`&#0j@'DbG&#^$PG.Ll+DNa<XCMKEV*N)LN/N"
  "*b=%Q6pia-Xg8I$<MR&,VdJe$<(7G;Ckl'&hF;;$<_=X(b.RS%%)###MPBuuE1V:v&cX&#2m#(&cV]`k9OhLMbn%s$G2,B$BfD3X*sp5#l,$R#]x_X1xKX%b5U*[r5iMfUo9U`N99hG)"
  "tm+/Us9pG)XPu`<0s-)WTt(gCRxIg(%6sfh=ktMKn3j)<6<b5Sk_/0(^]AaN#(p/L>&VZ>1i%h1S9u5o@YaaW$e+b<TWFn/Z:Oh(Cx2$lNEoN^e)#CFY@@I;BOQ*sRwZtZxRcU7uW6CX"
  "ow0i(?$Q[cjOd[P4d)]>ROPOpxTO7Stwi1::iB1q)C_=dV26J;2,]7op$]uQr@_V7$q^%lQwtuHY]=DX,n3L#0PHDO4f9>dC@O>HBuKPpP*E,N+b3L#lpR/MrTEH.IAQk.a>D[.e;mc."
  "x]Ip.PH^'/aqUO/$1WxLoW0[iLA<QT;5HKD+@qQ'NQ(3_PLhE48R.qAPSwQ0/WK?Z,[x?-J;jQTWA0X@KJ(_Y8N-:/M74:/-ZpKrUss?d#dZq]DAbkU*JqkL+nwX@@47`5>w=4h(9.`G"
  "CRUxHPeR`5Mjol(dUWxZa(>STrPkrJiWx`5U7F#.g*jrohGg`cg:lSTvEY/EV_7H4Q9[Z%cnv;JQYZ5q.l7Zeas:HOIZOB?G<Nald$qs]@]L<J7bR*>gv:[7MI2k).'2($5FNP&EQ(,)"
  "U]W]+fh18.vsai00);D3@4ku5P?DP8aJt+;qUM]=+b'8@;mViBKx0DE[-auGl8:PJ&Dj+M6OC]O^((##]`0i)drT;-7X`=-H3[igUnPG-NZlo.#k@h#=Ork$m>a>$-?Tm$UV(?#P6YY#"
  "'/###xe7q.73rI3*pP/$1>s9)W,JrM7SN]'/4C#v$U`0#V.[0>xQsH$fEmPMgY2u7Kh(G%siIfLSoS+MK2eTM$=5,M8p`A.;_R%#u[K#$x4AG8.kK/HSB==-'Ie/QTtG?-.*^N-4B/ZM"
  "_3YlQC7(p7q)&](`6_c)$/*JL(L-^(]$wIM`dPtOdGA,U3:w2M-0<q-]L_?^)1vw'.,MRsqVr.L;aN&#/EgJ)PBc[-f>+WomX2u7lqM2iEumMTcsF?-aT=Z-97UEnXglEn1K-bnEO`gu"
  "Ft(c%=;Am_Qs@jLooI&NX;]0#j4#F14;gl8-GQpgwhrq8'=l_f-b49'UOqkLu7-##oDY2L(te+Mch&gLYtJ,MEtJfLh'x'M=$CS-ZZ%P]8bZ>#S?YY#%Q&q'3^Fw&?D)UDNrocM3A76/"
  "/oL?#h7gl85[qW/NDOk%16ij;+:1a'iNIdb-ou8.P*w,v5#EI$TWS>Pot-R*H'-SEpA:g)f+O$%%`kA#G=8RMmG1&O`>to8bC]T&$,n.LoO>29sp3dt-52U%VM#q7'DHpg+#Z9%H[K<L"
  "%a2E-grWVM3@2=-k22tL]4$##6We'8UJCKE[d_=%wI;'6X-GsLX4j^SgJ$##R*w,vP3wK#iiW&#*h^D&R?jp7+/u&#(AP##XU8c$fSYW-J95_-Dp[g9wcO&#M-h1OcJlc-*vpw0xUX&#"
  "OQFKNX@QI'IoPp7nb,QU//MQ&ZDkKP)X<WSVL(68uVl&#c'[0#(s1X&xm$Y%B7*K:eDA323j998GXbA#pwMs-jgD$9QISB-A_(aN4xoFM^@C58D0+Q+q3n0#3U1InDjF682-SjMXJK)("
  "h$hxua_K]ul92%'BOU&#BRRh-slg8KDlr:%L71Ka:.A;%YULjDPmL<LYs8i#XwJOYaKPKc1h:'9Ke,g)b),78=I39B;xiY$bgGw-&.Zi9InXDuYa%G*f2Bq7mn9^#p1vv%#(Wi-;/Z5h"
  "o;#2:;%d&#x9v68C5g?ntX0X)pT`;%pB3q7mgGN)3%(P8nTd5L7GeA-GL@+%J3u2:(Yf>et`e;)f#Km8&+DC$I46>#Kr]]u-[=99tts1.qb#q72g1WJO81q+eN'03'eM>&1XxY-caEnO"
  "j%2n8)),?ILR5^.Ibn<-X-Mq7[a82Lq:F&#ce+S9wsCK*x`569E8ew'He]h:sI[2LM$[guka3ZRd6:t%IG:;$%YiJ:Nq=?eAw;/:nnDq0(CYcMpG)qLN4$##&J<j$UpK<Q4a1]MupW^-"
  "sj_$%[HK%'F####QRZJ::Y3EGl4'@%FkiAOg#p[##O`gukTfBHagL<LHw%q&OV0##F=6/:chIm0@eCP8X]:kFI%hl8hgO@RcBhS-@Qb$%+m=hPDLg*%K8ln(wcf3/'DW-$.lR?n[nCH-"
  "eXOONTJlh:.RYF%3'p6sq:UIMA945&^HFS87@$EP2iG<-lCO$%c`uKGD3rC$x0BL8aFn--`ke%#HMP'vh1/R&O_J9'um,.<tx[@%wsJk&bUT2`0uMv7gg#qp/ij.L56'hl;.s5CUrxjO"
  "M7-##.l+Au'A&O:-T72L]P`&=;ctp'XScX*rU.>-XTt,%OVU4)S1+R-#dg0/Nn?Ku1^0f$B*P:Rowwm-`0PKjYDDM'3]d39VZHEl4,.j']Pk-M.h^&:0FACm$maq-&sgw0t7/6(^xtk%"
  "LuH88Fj-ekm>GA#_>568x6(OFRl-IZp`&b,_P'$M<Jnq79VsJW/mWS*PUiq76;]/NM_>hLbxfc$mj`,O;&%W2m`Zh:/)Uetw:aJ%]K9h:TcF]u_-Sj9,VK3M.*'&0D[Ca]J9gp8,kAW]"
  "%(?A%R$f<->Zts'^kn=-^@c4%-pY6qI%J%1IGxfLU9CP8cbPlXv);C=b),<2mOvP8up,UVf3839acAWAW-W?#ao/^#%KYo8fRULNd2.>%m]UK:n%r$'sw]J;5pAoO_#2mO3n,'=H5(et"
  "Hg*`+RLgv>=4U8guD$I%D:W>-r5V*%j*W:Kvej.Lp$<M-SGZ':+Q_k+uvOSLiEo(<aD/K<CCc`'Lx>'?;++O'>()jLR-^u68PHm8ZFWe+ej8h:9r6L*0//c&iH&R8pRbA#Kjm%upV1g:"
  "a_#Ur7FuA#(tRh#.Y5K+@?3<-8m0$PEn;J:rh6?I6uG<-`wMU'ircp0LaE_OtlMb&1#6T.#FDKu#1Lw%u%+GM+X'e?YLfjM[VO0MbuFp7;>Q&#WIo)0@F%q7c#4XAXN-U&VB<HFF*qL("
  "$/V,;(kXZejWO`<[5?\?ewY(*9=%wDc;,u<'9t3W-(H1th3+G]ucQ]kLs7df($/*JL]@*t7Bu_G3_7mp7<iaQjO@.kLg;x3B0lqp7Hf,^Ze7-##@/c58Mo(3;knp0%)A7?-W+eI'o8)b<"
  "nKnw'Ho8C=Y>pqB>0ie&jhZ[?iLR@@_AvA-iQC(=ksRZRVp7`.=+NpBC%rh&3]R:8XDmE5^V8O(x<<aG/1N$#FX$0V5Y6x'aErI3I$7x%E`v<-BY,)%-?Psf*l?%C3.mM(=/M0:JxG'?"
  "7WhH%o'a<-80g0NBxoO(GH<dM]n.+%q@jH?f.UsJ2Ggs&4<-e47&Kl+f//9@`b+?.TeN_&B8Ss?v;^Trk;f#YvJkl&w$]>-+k?'(<S:68tq*WoDfZu';mM?8X[ma8W%*`-=;D.(nc7/;"
  ")g:T1=^J$&BRV(-lTmNB6xqB[@0*o.erM*<SWF]u2=st-*(6v>^](H.aREZSi,#1:[IXaZFOm<-ui#qUq2$##Ri;u75OK#(RtaW-K-F`S+cF]uN`-KMQ%rP/Xri.LRcB##=YL3BgM/3M"
  "D?@f&1'BW-)Ju<L25gl8uhVm1hL$##*8###'A3/LkKW+(^rWX?5W_8g)a(m&K8P>#bmmWCMkk&#TR`C,5d>g)F;t,4:@_l8G/5h4vUd%&%950:VXD'QdWoY-F$BtUwmfe$YqL'8(PWX("
  "P?^@Po3$##`MSs?DWBZ/S>+4%>fX,VWv/w'KD`LP5IbH;rTV>n3cEK8U#bX]l-/V+^lj3;vlMb&[5YQ8#pekX9JP3XUC72L,,?+Ni&co7ApnO*5NK,((W-i:$,kp'UDAO(G0Sq7MVjJs"
  "bIu)'Z,*[>br5fX^:FPAWr-m2KgL<LUN098kTF&#lvo58=/vjDo;.;)Ka*hLR#/k=rKbxuV`>Q_nN6'8uTG&#1T5g)uLv:873UpTLgH+#FgpH'_o1780Ph8KmxQJ8#H72L4@768@Tm&Q"
  "h4CB/5OvmA&,Q&QbUoi$a_%3M01H)4x7I^&KQVgtFnV+;[Pc>[m4k//,]1?#`VY[Jr*3&&slRfLiVZJ:]?=K3Sw=[$=uRB?3xk48@aeg<Z'<$#4H)6,>e0jT6'N#(q%.O=?2S]u*(m<-"
  "V8J'(1)G][68hW$5'q[GC&5j`TE?m'esFGNRM)j,ffZ?-qx8;->g4t*:CIP/[Qap7/9'#(1sao7w-.qNUdkJ)tCF&#B^;xGvn2r9FEPFFFcL@.iFNkTve$m%#QvQS8U@)2Z+3K:AKM5i"
  "sZ88+dKQ)W6>J%CL<KE>`.d*(B`-n8D9oK<Up]c$X$(,)M8Zt7/[rdkqTgl-0cuGMv'?>-XV1q['-5k'cAZ69e;D_?$ZPP&s^+7])$*$#@QYi9,5P&#9r+$%CE=68>K8r0=dSC%%(@p7"
  ".m7jilQ02'0-VWAg<a/''3u.=4L$Y)6k/K:_[3=&jvL<L0C/2'v:^;-DIBW,B4E68:kZ;%?8(Q8BH=kO65BW?xSG&#@uU,DS*,?.+(o(#1vCS8#CHF>TlGW'b)Tq7VT9q^*^$$.:&N@@"
  "$&)WHtPm*5_rO0&e%K&#-30j(E4#'Zb.o/(Tpm$>K'f@[PvFl,hfINTNU6u'0pao7%XUp9]5.>%h`8_=VYbxuel.NTSsJfLacFu3B'lQSu/m6-Oqem8T+oE--$0a/k]uj9EwsG>%veR*"
  "hv^BFpQj:K'#SJ,sB-'#](j.Lg92rTw-*n%@/;39rrJF,l#qV%OrtBeC6/,;qB3ebNW[?,Hqj2L.1NP&GjUR=1D8QaS3Up&@*9wP?+lo7b?@%'k4`p0Z$22%K3+iCZj?XJN4Nm&+YF]u"
  "@-W$U%VEQ/,,>>#)D<h#`)h0:<Q6909ua+&VU%n2:cG3FJ-%@Bj-DgLr`Hw&HAKjKjseK</xKT*)B,N9X3]krc12t'pgTV(Lv-tL[xg_%=M_q7a^x?7Ubd>#%8cY#YZ?=,`Wdxu/ae&#"
  "w6)R89tI#6@s'(6Bf7a&?S=^ZI_kS&ai`&=tE72L_D,;^R)7[$s<Eh#c&)q.MXI%#v9ROa5FZO%sF7q7Nwb&#ptUJ:aqJe$Sl68%.D###EC><?-aF&#RNQv>o8lKN%5/$(vdfq7+ebA#"
  "u1p]ovUKW&Y%q]'>$1@-[xfn$7ZTp7mM,G,Ko7a&Gu%G[RMxJs[0MM%wci.LFDK)(<c`Q8N)jEIF*+?P2a8g%)$q]o2aH8C&<SibC/q,(e:v;-b#6[$NtDZ84Je2KNvB#$P5?tQ3nt(0"
  "d=j.LQf./Ll33+(;q3L-w=8dX$#WF&uIJ@-bfI>%:_i2B5CsR8&9Z&#=mPEnm0f`<&c)QL5uJ#%u%lJj+D-r;BoF&#4DoS97h5g)E#o:&S4weDF,9^Hoe`h*L+_a*NrLW-1pG_&2UdB8"
  "6e%B/:=>)N4xeW.*wft-;$'58-ESqr<b?UI(_%@[P46>#U`'6AQ]m&6/`Z>#S?YY#Vc;r7U2&326d=w&H####?TZ`*4?&.MK?LP8Vxg>$[QXc%QJv92.(Db*B)gb*BM9dM*hJMAo*c&#"
  "b0v=Pjer]$gG&JXDf->'StvU7505l9$AFvgYRI^&<^b68?j#q9QX4SM'RO#&sL1IM.rJfLUAj221]d##DW=m83u5;'bYx,*Sl0hL(W;;$doB&O/TQ:(Z^xBdLjL<Lni;''X.`$#8+1GD"
  ":k$YUWsbn8ogh6rxZ2Z9]%nd+>V#*8U_72Lh+2Q8Cj0i:6hp&$C/:p(HK>T8Y[gHQ4`4)'$Ab(Nof%V'8hL&#<NEdtg(n'=S1A(Q1/I&4([%dM`,Iu'1:_hL>SfD07&6D<fp8dHM7/g+"
  "tlPN9J*rKaPct&?'uBCem^jn%9_K)<,C5K3s=5g&GmJb*[SYq7K;TRLGCsM-$$;S%:Y@r7AK0pprpL<Lrh,q7e/%KWK:50I^+m'vi`3?%Zp+<-d+$L-Sv:@.o19n$s0&39;kn;S%BSq*"
  "$3WoJSCLweV[aZ'MQIjO<7;X-X;&+dMLvu#^UsGEC9WEc[X(wI7#2.(F0jV*eZf<-Qv3J-c+J5AlrB#$p(H68LvEA'q3n0#m,[`*8Ft)FcYgEud]CWfm68,(aLA$@EFTgLXoBq/UPlp7"
  ":d[/;r_ix=:TF`S5H-b<LI&HY(K=h#)]Lk$K14lVfm:x$H<3^Ql<M`$OhapBnkup'D#L$Pb_`N*g]2e;X/Dtg,bsj&K#2[-:iYr'_wgH)NUIR8a1n#S?Yej'h8^58UbZd+^FKD*T@;6A"
  "7aQC[K8d-(v6GI$x:T<&'Gp5Uf>@M.*J:;$-rv29'M]8qMv-tLp,'886iaC=Hb*YJoKJ,(j%K=H`K.v9HggqBIiZu'QvBT.#=)0ukruV&.)3=(^1`o*Pj4<-<aN((^7('#Z0wK#5GX@7"
  "u][`*S^43933A4rl][`*O4CgLEl]v$1Q3AeF37dbXk,.)vj#x'd`;qgbQR%FW,2(?LO=s%Sc68%NP'##Aotl8x=BE#j1UD([3$M(]UI2LX3RpKN@;/#f'f/&_mt&F)XdF<9t4)Qa.*kT"
  "LwQ'(TTB9.xH'>#MJ+gLq9-##@HuZPN0]u:h7.T..G:;$/Usj(T7`Q8tT72LnYl<-qx8;-HV7Q-&Xdx%1a,hC=0u+HlsV>nuIQL-5<N?)NBS)QN*_I,?&)2'IM%L3I)X((e/dl2&8'<M"
  ":^#M*Q+[T.Xri.LYS3v%fF`68h;b-X[/En'CR.q7E)p'/kle2HM,u;^%OKC-N+Ll%F9CF<Nf'^#t2L,;27W:0O@6##U6W7:$rJfLWHj$#)woqBefIZ.PK<b*t7ed;p*_m;4ExK#h@&]>"
  "_>@kXQtMacfD.m-VAb8;IReM3$wf0''hra*so568'Ip&vRs849'MRYSp%:t:h5qSgwpEr$B>Q,;s(C#$)`svQuF$##-D,##,g68@2[T;.XSdN9Qe)rpt._K-#5wF)sP'##p#C0c%-Gb%"
  "hd+<-j'Ai*x&&HMkT]C'OSl##5RG[JXaHN;d'uA#x._U;.`PU@(Z3dt4r152@:v,'R.Sj'w#0<-;kPI)FfJ&#AYJ&#//)>-k=m=*XnK$>=)72L]0I%>.G690a:$##<,);?;72#?x9+d;"
  "^V'9;jY@;)br#q^YQpx:X#Te$Z^'=-=bGhLf:D6&bNwZ9-ZD#n^9HhLMr5G;']d&6'wYmTFmL<LD)F^%[tC'8;+9E#C$g%#5Y>q9wI>P(9mI[>kC-ekLC/R&CH+s'B;K-M6$EB%is00:"
  "+A4[7xks.LrNk0&E)wILYF@2L'0Nb$+pv<(2.768/FrY&h$^3i&@+G%JT'<-,v`3;_)I9M^AE]CN?Cl2AZg+%4iTpT3<n-&%H%b<FDj2M<hH=&Eh<2Len$b*aTX=-8QxN)k11IM1c^j%"
  "9s<L<NFSo)B?+<-(GxsF,^-Eh@$4dXhN$+#rxK8'je'D7k`e;)2pYwPA'_p9&@^18ml1^[@g4t*[JOa*[=Qp7(qJ_oOL^('7fB&Hq-:sf,sNj8xq^>$U4O]GKx'm9)b@p7YsvK3w^YR-"
  "CdQ*:Ir<($u&)#(&?L9Rg3H)4fiEp^iI9O8KnTj,]H?D*r7'M;PwZ9K0E^k&-cpI;.p/6_vwoFMV<->#%Xi.LxVnrU(4&8/P+:hLSKj$#U%]49t'I:rgMi'FL@a:0Y-uA[39',(vbma*"
  "hU%<-SRF`Tt:542R_VV$p@[p8DV[A,?1839FWdF<TddF<9Ah-6&9tWoDlh]&1SpGMq>Ti1O*H&#(AL8[_P%.M>v^-))qOT*F5Cq0`Ye%+$B6i:7@0IX<N+T+0MlMBPQ*Vj>SsD<U4JHY"
  "8kD2)2fU/M#$e.)T4,_=8hLim[&);?UkK'-x?'(:siIfL<$pFM`i<?%W(mGDHM%>iWP,##P`%/L<eXi:@Z9C.7o=@(pXdAO/NLQ8lPl+HPOQa8wD8=^GlPa8TKI1CjhsCTSLJM'/Wl>-"
  "S(qw%sf/@%#B6;/U7K]uZbi^Oc^2n<bhPmUkMw>%t<)'mEVE''n`WnJra$^TKvX5B>;_aSEK',(hwa0:i4G?.Bci.(X[?b*($,=-n<.Q%`(X=?+@Am*Js0&=3bh8K]mL<LoNs'6,'85`"
  "0?t/'_U59@]ddF<#LdF<eWdF<OuN/45rY<-L@&#+fm>69=Lb,OcZV/);TTm8VI;?%OtJ<(b4mq7M6:u?KRdF<gR@2L=FNU-<b[(9c/ML3m;Z[$oF3g)GAWqpARc=<ROu7cL5l;-[A]%/"
  "+fsd;l#SafT/f*W]0=O'$(Tb<[)*@e775R-:Yob%g*>l*:xP?Yb.5)%w_I?7uk5JC+FS(m#i'k.'a0i)9<7b'fs'59hq$*5Uhv##pi^8+hIEBF`nvo`;'l0.^S1<-wUK2/Coh58KKhLj"
  "M=SO*rfO`+qC`W-On.=AJ56>>i2@2LH6A:&5q`?9I3@@'04&p2/LVa*T-4<-i3;M9UvZd+N7>b*eIwg:CC)c<>nO&#<IGe;__.thjZl<%w(Wk2xmp4Q@I#I9,DF]u7-P=.-_:YJ]aS@V"
  "?6*C()dOp7:WL,b&3Rg/.cmM9&r^>$(>.Z-I&J(Q0Hd5Q%7Co-b`-c<N(6r@ip+AurK<m86QIth*#v;-OBqi+L7wDE-Ir8K['m+DDSLwK&/.?-V%U_%3:qKNu$_b*B-kp7NaD'QdWQPK"
  "Yq[@>P)hI;*_F]u`Rb[.j8_Q/<&>uu+VsH$sM9TA%?)(vmJ80),P7E>)tjD%2L=-t#fK[%`v=Q8<FfNkgg^oIbah*#8/Qt$F&:K*-(N/'+1vMB,u()-a.VUU*#[e%gAAO(S>WlA2);Sa"
  ">gXm8YB`1d@K#n]76-a$U,mF<fX]idqd)<3,]J7JmW4`6]uks=4-72L(jEk+:bJ0M^q-8Dm_Z?0olP1C9Sa&H[d&c$ooQUj]Exd*3ZM@-WGW2%s',B-_M%>%Ul:#/'xoFM9QX-$.QN'>"
  "[%$Z$uF6pA6Ki2O5:8w*vP1<-1`[G,)-m#>0`P&#eb#.3i)rtB61(o'$?X3B</R90;eZ]%Ncq;-Tl]#F>2Qft^ae_5tKL9MUe9b*sLEQ95C&`=G?@Mj=wh*'3E>=-<)Gt*Iw)'QG:`@I"
  "wOf7&]1i'S01B+Ev/Nac#9S;=;YQpg_6U`*kVY39xK,[/6Aj7:'1Bm-_1EYfa1+o&o4hp7KN_Q(OlIo@S%;jVdn0'1<Vc52=u`3^o-n1'g4v58Hj&6_t7$##?M)c<$bgQ_'SY((-xkA#"
  "Y(,p'H9rIVY-b,'%bCPF7.J<Up^,(dU1VY*5#WkTU>h19w,WQhLI)3S#f$2(eb,jr*b;3Vw]*7NH%$c4Vs,eD9>XW8?N]o+(*pgC%/72LV-u<Hp,3@e^9UB1J+ak9-TN/mhKPg+AJYd$"
  "MlvAF_jCK*.O-^(63adMT->W%iewS8W6m2rtCpo'RS1R84=@paTKt)>=%&1[)*vp'u+x,VrwN;&]kuO9JDbg=pO$J*.jVe;u'm0dr9l,<*wMK*Oe=g8lV_KEBFkO'oU]^=[-792#ok,)"
  "i]lR8qQ2oA8wcRCZ^7w/Njh;?.stX?Q1>S1q4Bn$)K1<-rGdO'$Wr.Lc.CG)$/*JL4tNR/,SVO3,aUw'DJN:)Ss;wGn9A32ijw%FL+Z0Fn.U9;reSq)bmI32U==5ALuG&#Vf1398/pVo"
  "1*c-(aY168o<`JsSbk-,1N;$>0:OUas(3:8Z972LSfF8eb=c-;>SPw7.6hn3m`9^Xkn(r.qS[0;T%&Qc=+STRxX'q1BNk3&*eu2;&8q$&x>Q#Q7^Tf+6<(d%ZVmj2bDi%.3L2n+4W'$P"
  "iDDG)g,r%+?,$@?uou5tSe2aN_AQU*<h`e-GI7)?OK2A.d7_c)?wQ5AS@DL3r#7fSkgl6-++D:'A,uq7SvlB$pcpH'q3n0#_%dY#xCpr-l<F0NR@-##FEV6NTF6##$l84N1w?AO>'IAO"
  "URQ##V^Fv-XFbGM7Fl(N<3DhLGF%q.1rC$#:T__&Pi68%0xi_&[qFJ(77j_&JWoF.V735&T,[R*:xFR*K5>>#`bW-?4Ne_&6Ne_&6Ne_&n`kr-#GJcM6X;uM6X;uM(.a..^2TkL%oR(#"
  ";u.T%fAr%4tJ8&><1=GHZ_+m9/#H1F^R#SC#*N=BA9(D?v[UiFY>>^8p,KKF.W]L29uLkLlu/+4T<XoIB&hx=T1PcDaB&;HH+-AFr?(m9HZV)FKS8JCw;SD=6[^/DZUL`EUDf]GGlG&>"
  "w$)F./^n3+rlo+DB;5sIYGNk+i1t-69Jg--0pao7Sm#K)pdHW&;LuDNH@H>#/X-TI(;P>#,Gc>#0Su>#4`1?#8lC?#<xU?#@.i?#D:%@#HF7@#LRI@#P_[@#Tkn@#Xw*A#]-=A#a9OA#"
  "d<F&#*;G##.GY##2Sl##6`($#:l:$#>xL$#B.`$#F:r$#JF.%#NR@%#R_R%#Vke%#Zww%#_-4&#3^Rh%Sflr-k'MS.o?.5/sWel/wpEM0%3'/1)K^f1-d>G21&v(35>V`39V7A4=onx4"
  "A1OY5EI0;6Ibgr6M$HS7Q<)58C5w,;WoA*#[%T*#`1g*#d=#+#hI5+#lUG+#pbY+#tnl+#x$),#&1;,#*=M,#.I`,#2Ur,#6b.-#;w[H#iQtA#m^0B#qjBB#uvTB##-hB#'9$C#+E6C#"
  "/QHC#3^ZC#7jmC#;v)D#?,<D#C8ND#GDaD#KPsD#O]/E#g1A5#KA*1#gC17#MGd;#8(02#L-d3#rWM4#Hga1#,<w0#T.j<#O#'2#CYN1#qa^:#_4m3#o@/=#eG8=#t8J5#`+78#4uI-#"
  "m3B2#SB[8#Q0@8#i[*9#iOn8#1Nm;#^sN9#qh<9#:=x-#P;K2#$%X9#bC+.#Rg;<#mN=.#MTF.#RZO.#2?)4#Y#(/#[)1/#b;L/#dAU/#0Sv;#lY$0#n`-0#sf60#(F24#wrH0#%/e0#"
  "TmD<#%JSMFove:CTBEXI:<eh2g)B,3h2^G3i;#d3jD>)4kMYD4lVu`4m`:&5niUA5@(A5BA1]PBB:xlBCC=2CDLXMCEUtiCf&0g2'tN?PGT4CPGT4CPGT4CPGT4CPGT4CPGT4CPGT4CP"
  "GT4CPGT4CPGT4CPGT4CPGT4CPGT4CP-qekC`.9kEg^+F$kwViFJTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5o,^<-28ZI'O?;xp"
  "O?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xp;7q-#lLYI:xvD=#";

static const char res__ico_fnt[] =
  "7])#######n1Y-l'/###W),##2(V$#Q6>##.FxF>6pY(/Q5)=-'OE/1I[n422x6mBj,>>#-rEn/aNV=Bb$W,W2tEn/-I:;$F2XGH`$vhLukKv/4EXQ'$jg0F0&'+MujT,5=9>n&%J%/G"
  "E#l2(W>L5MKWa,Dk$[KtD0/(#XQ*1#8J9HDtHN$/de./#4qluL`$tLD6q;W%ZCcwLH(i%M<x$KDf+ODn7$#)MmVZ)M.xI`Ew]CEHZwo##_d^w'?dX3Fo;>J#ZKI0#d3m9D;4^=Bbbw`U"
  "?c[w'4@2a#D&S+HL;WdR4@(;?06YY#+eFVCYV;36C0LcDjaDE-uuF%4^D-(#RApl8`Yx+#twCEHVle%#JQr.MtwSfL9b@6/b1+7DeA=gL345GMLLH##,x:T.,),##:3^g1*VCv#Q7B2#"
  "*5YY#41O$Mi.BrM1.RxkW$eY#37c&#kg3SRc:*oC8q9`adYnr682'DE-Y4;Z>Ujk=4rsh#I)###X^j-$6>K21,C?\?$J-HlfZ2c'&hFFrd_Du'&NM-F%>.GR*=pCfUDt_>$7F(B#9me+M"
  "1%g+M?e(eM,*^fLJ4pfLGYZ##7[Qk%RD35&:[jl&>tJM'B6,/(FNcf(JgCG)N)%)*RA[`*VY<A+Zrsx+_4TY,cL5;-gelr-k'MS.o?.5/sWel/wpEM0%3'/1)K^f1-d>G21&v(35>V`3"
  "9V7A4=onx4A1OY5EI0;6Ibgr6c6%##:q+/(%2G>#(5JfLKhV;$/YWh#07%##1xS,MtqJfL$Fl(NICZY#FX9.$uxefL1flS.'/###'flS.(2###'flS.)5###'flS.*8###$JpV-dYC_&"
  "#JO&#@Uu)NQ645&:49.$'SXgL.JpV-hfC_&#JO&#DUu)NUZKM':49.$+l'hL.JpV-lrC_&#JO&#HUu)NY)df(:49.$/.LhL.JpV-p(D_&#JO&#LUu)N^M%)*:49.$3FqhL.JpV-t4D_&"
  "#JO&#PUu)Nbr<A+:49.$7_?iL.JpV-x@D_&#JO&#TUu)Nf@TY,:49.$;wdiL.JpV-&MD_&#JO&#XUu)Njelr-:49.$?93jL.JpV-*YD_&#JO&#]Uu)Nn3.5/:49.$CQWjL.JpV-.fD_&"
  "#JO&#aUu)NrWEM0:49.$Gj&kL.JpV-2rD_&#JO&#eUu)Nv&^f1:49.$K,KkL.JpV-6(E_&#JO&#iUu)N$Ku(3:49.$ODpkL.JpV-:4E_&#JO&#mUu)N(p6A4:49.$S]>lL.JpV->@E_&"
  "#JO&#qUu)N,>NY5:49.$WuclL.JpV-BLE_&#JO&#uUu)N0cfr6:49.$`<-87$&###m.ecVENCG)9J5N)';P>#U05##U`'##G:^)4#3W&+uA3I):2Cv-oUFb3sJ))3#VYV-f(]cVM/Er?"
  "t93L3U17F#A?pV-TdSe$qH,`#RNTO0*X%##%,Y:vha-iLo*t$#dp+X7SYU;$WiB>#(8P>#1qP$B@,?;$-T1v>-Y(Z#3TL;$'G9^5b3:]$q+rB#U*TM'AkQA/=V&E#nGpkLNIng)&6Hj'"
  "TpVa4#Z'B$xEWt*?>A(WemPJ(tnwx.aoX[,H&.+4xd`n#V4OcMPCWe3h</Q(Y'ko/2)_e$4*9j34gY8%b+geZDq)##)&>uuwgv$$uSl##Pqn%#ToF6#Gl'e72$pq#Rlv$AXg2mfS4B##"
  "4mIYV'W/:9ne;?#a<@e7d;VY5Z`:Z#FmeP&b,f?nYs*h:PSIK:?Rap%>FIW$VZ:Z#97n8%]#M$#Bbe<$HMNT%.K;,#6P,##==LcD,Vu>#6,Y?#.i_v#=Px-#?L.w#-Q:v#EVQP&iD]_m"
  "@,GoYO9Q4F[%4'@dt(o035OI)lP`k'r.rB#PDRv$a&SF4;$QQ's%m'&1;at(SH()34mWI)?7%s$nbK+*M@i?#W%E_&^Ko8%x3vr-w[<9/$;Ls-/W8f3&,]]4RWgf1Ac``3%&AA4#/<9/"
  "fPpG3XS`$'MP,G4Rq@.*q%^F*/p^I*<07IMg^YA#X?hc)s35b@D1T;.HDav-a=@8%k,uB8Jkw`4GXI)4Mq>`-HH@e6(>m=7m*[2Mp*0_OxxMT/QrX9VfKXk5<4j<>sDQh#'tLA1^*OM0"
  "gkl(N8B^Ki7s3&%f;U>$2sA6(>*;+*AqS2'S=53'>q>B$5<iC$b5$3'75-T%8#FA%X`vp/$G'T7[$o8%GmO?--AwW%]&`2'@_,a*Ok<t$0du>#BHm5&1xSE#fuVS&nQf/1o05'$OZ5^="
  "O&>?,l)'@#kl;K(l&c=$1L1#,KB.+NNx&X%N<`>$e*vA#^JJ-dN,NV,C30$-A]._#<tZ>5+4nseF&D?#w0)`+_LR<$WrD)36>$d+KcgHQBDW@$186X$i[Oq&`h`?#J._b$P?.Z$W5U<."
  "6;_G+Rv&T&@6O<7aRpf)kSmn&Q3-lf<LYj'+H0[#88_r.e>da3PemY#@ptm'6g3x%xB1-M7P*s%fVG>#5%RP/Gmx5'rmKD3&#,44^E;O+_+ZLM&g;k'X]h@-a4e=.W7QP/b7AT.-5YY#"
  "8:$DE*(NA=AdZ`*dGr80(Auu#t#xu#ZsJI$%),##Y:j'%pFU/#c4fL(shFgL6D/x$p>(l0kH7x,Vs'Y$0qwX-Unn8%_%NT/+M>c49XH8'uIMT%`pM,*36RG*iK%],5<,-2onmC#v$S;m"
  "`(t**Iq)l]6FL#$Kdl8.(PUV$2n)q/NWw.:+M-TB[?.%$U0,p%4u._#mBrD+2o_;$fhK&#mj[8&+WSW%GO_,DCxMd%pOns$<Nc##7+%<$<T1Z#,?;,#9DNPA,Y1Z#s4)N9n1ZG4-fj9/"
  "A:_f)V$F9%no<i(wD1l0XsHZ-eaFx#eE'P(@cs'4m>:Z-jBFA#Lov[-u?lD#I4L#$EN/R.W*w5/E+p?-NmlYub(96&9WD:MGE=30,Gr2'<_ET%<q@K*$BI1gfX#JC/AU1MjH.QOagt5&"
  "qxH['q3n0#asoXu7U^PAU4LS./j>G2otqf)8V(Z#qE,+#WoRfL`TMcD-VL;$Z/uoLgNN;$5N(Z#OgVF*+(-(+wc_l0W:AC#mP1I$I@Gm'():I$+r$x,^s8I$$#H=7nM9I$0s9M)-><j1"
  "AS))3ql@JLWf#@&3_ZZ,9Q4U&3(lU&Yo>[&3[ZZ,CT8U)iFvGidYWSd$gvS`2;u/l@_.u-3-7ENW+TgLfwe6/`>uOHvDjDMOF$##&&>uua9LhLD:d&#Dc/*#sE,i*>EL,))Duu#+oBj%"
  "dL,+#6(7s$U0[0#+Mc##V<[0#6lq;$I%Ub$*AP>#'8>##DA<T%4E;,#Co/QA:X<T%)GZ,4q&>:.Npp;.(v$m0Sg'u$I=^v-Ut;8.X5Gd3vf%&4Z;5-35NCD3:sIL((sCm/f@0+*oGSF4"
  "3[D>&3UL,3tD1C&WM_T%teS#cu1rC&U-Cu.VmQd3*S2B#@ZO4:IHHc2g*HZ8B5csArk7Y1rko>csATAcZr*GM+,@]-'ArU2[TdlA)P.R&5jou,0);D3HI<?#'2G>#uAP##-V1Z#dmRfL"
  "'o-##<ZU;$,9;,#7JaPA.fU;$ut_TrAJ[v$a7&_od]tfM5RqkL,m(o-ucCtUvUcj%o<uj.aS)<%mOv@M=+I.q-m%)NfITY,e$W)+l=ofLSo)($Am@^#Er'kbPS,%,UX@s$L9@K)[(Ls-"
  "3GO,MPKZd3d^s/M_Da.3SiY1M5KihLCIL+*UXrH;j)%bWpE#W-`T5F%AlEx'iwpaW^hv4*H@Fk'e<qWQT,;h13#nERW>uu#dk53#@kMV7MV,##I3SnLaGYcMEB@C#S$###.KkI)qPCl0"
  "mw4gLT'MB#T4gA#W_l#$Mt=V-EfS]uo:/b-O2+LPtuA+*EsOm'#UHIMap?'X&l4hLrHf-#,&>uu3Y.d#@7>##MXI%#^3=&#<Ja)#X%T*#vnC$1q(Jv57u(Z#ZAOe)>L%@#AO[s$Wu<5&"
  "D<K6&i#Ml'MtxM':UwW$RIr@#>.%[#bd'7)*J(v#FU8r%A%[S%(>Y>#ldOU%pAWPA+S1v#;$_Z7;Y[[->OZc*iZbX$l4^l8<mrT9,hTZ7#x.E5OABj2'Q:?-jJu',FBo],6=Qc*IK.iL"
  "mEL+*SU@<$&=LhLihdJ(1E1l0skY)4XEW@,j^Aj0a+O_$HPjc)e#NhLJ6,H2v(V:%Ynn8%a%NT//,-J*7Lm<-GdF?-f1jq/7Sn%OliiO're$^4QE15KQ@Mcu+k[**,pxZP^,.)*5fMh'"
  "7B=m'mi*n/_4Dgufi7q9R211M.6M*:'h02:vA[(:xZ%RN5vWuLj.3ulxFClL@fdi9FeY&#xnL?@S6O6)aX18.0-$5A]Hx7Ip2>SIC]M@#U$?4EIQ4;H(Z+C$mwu@/_.jMKoqA6&:A*9%"
  "(QTS%>O<T%TYB>#*DP>#CD>?#.l6s$:Dx-#>nSQ&Gf4.#=@<T%Ut%x&)2###^qi*%T8<PA,<(<-SSO`+ejZ&J0PQc4+cjT/Jm';.PC'(42d8^+o9OA#jD<r&me_F*CCS:/7c,0)J]d8/"
  "]$vM(2.^C4abL+*irv;%5C;=-7'^,2]UUa4%[*G4FxJC4K^Rl1bu?#6R@=K2KB0u$xS?A4'U1,)XJ=c4uppo/*W8f3fd<c4?0QD-5I(T020e,*nwk%$oZxk9tU#B=t5Ro'LQl8%3k,in"
  "X(-Yu6-+#)).TD=Fx1?#gNIa+n0d#Be*,9%<c:Z#?$LX%fBwx,sKtT%N^Yn&FL[W$-l?8%Q32X,pZk/i7Y(v#Wu[fLU%bN;1GNp%C@Mw,M4D<']8IV%a]Q<(A,3iKt7WT%xtj6/hVI8["
  "/UOm#)?Rqnis*,;bc&^+ClcY#nG)4'74a8%PYjE*-.+T%>&,=7:mA_>jW^m&WN-)*>dacu..%1*`O'_*pee[#@JlY#7rlYu8]4X$i`jM2_=,gLjniY&Ma*W6+WH(#3q5Q)ThiM0]0s20"
  "EA.?$*ORG;57@W$X3;#n`tZY#0oLZ#@MUS%;1[s$OX)%&v#*gLPctO%lJ5b+tTcn014T:/o`'l%m=+gj^N[g)s46J*RWc8/+gB.*g;M#RJ[#,MD,/u9`fSc18/E&7cv?_-cg5WEI3N*:"
  "Rb[v?b####%HpS%UU?##5f1$#UKb&#/pm(#mw1G#tn]T.1uU?#rDnf(lW0[[kkj]7$K<X(48)m&B8^Y#MHsxFF%Is$<=e8%$P4]#Ihn8%hFro%>o:$#`s^n))J:;$=7:,DH72v#:x6W$"
  ">Mx-#]G3.)xoML+GB,/(js3eDTt/U%jJcoLlG1U%D1Yj'6[j.;%fa9DC9j:DA5FZgO++TB0)4^#tL,)h(3,^?M/ON;&lGq&C:.<$xKN`<^U-P;M&sT9e)8p0H/,G4GdE.3*fj9/Kt2)+"
  "adGn&U9tT%K-B6&;(%<$%oP29f0LB#R1UF*uMk+N0T'r.cPwV%OHF:.6L/@#$DXI)*kRP/QV>c4)`OF3K<:)3+/rv-I_[@#R:8>:uOkA#FRS[+2mQ6:t[u%F21aUK/`Am&0$n6't6XP0"
  "_1s^#kBnv9C;]6:jGg;-w9*S,?B)*ux-nlbW>^w6HsVR'WrM':iqYIqC?ihL?N4D<F@NeFU$of$/ws+;4;$p%-oI@#^ldC#)IIG;xa-^4r%%w#>R<9%fMwo%?UEp%UA[S%PWS#G'C,H2"
  "(Duu#D41,D0oqr$KUL,Dl/Uh$-OdZ$.E&##fZT=8BLJ88w]]v$g+,']gh4W8lX'_SPT^1QM7t]7nAu'6<]A`=lBEm;@Xk'6`^[12.HnY%&S)'64m.)*edB:%HXX/2a)DK1+Sx*3Ab#V/"
  "b.i?#E'.L,4cK+*XIDX-(lNX(^K&o80F5K1MW^Q&61Rs$16)6&1=Iw@<$GR&0CRZ,+gr#5sb&m&[t/R&XXZZ,(L@A,bN&3M,)nC#Pw:ZuZkc>#BlZT%Smap@_2_Z'pQ+Z&du`S&lD0t&"
  "PaG^=#9Qo3cU:%,m>vC-`E&%#v7?68u7CW-Dc/*#'LTP)j%O87q4[s$.>)6&51R8%_:S/L2rLv#Kn@jMZn;Z#+nS^ZSuHW.3Uov7'I-H3nxQ^Zhk)/:I5Z;%mI2W-tI)QLhl4c4U7*u7"
  "&QLjC]PcB,QeuS.$4fQ&*tFg.XAj8%wSq#.2_jB,B.iB#IgTe$^r:ZuIql>#/NUXRtTen(/qs+;%im%=r;L@#CQ2614o:Z#-cBW/Vh6Q8;$g*%JORW-I]MWfCe$68fYL/)2M6<-8Xpk$"
  "XjVa4ktr?#cJ?C#=V&E#;lm=72q_F*7P[]4?R$v,Jn@8%]_QP/Oaxu6Wj<X$Z:lfC@s-s$Xb*X$_)MZ#D5&NKXu]#,aR0)Nxs]w#ft1?#0cjT%HV#<-==^A.j^$d)p?$aNSO`].AI>#Y"
  ")>58..qhV$Aeq^#+9`G#_?Hlf%2TZ$7lcYuSS,<-qr?.%h,dxb_Z=g$YxGg3Ul$W$-/5##sH,+#/VCv#l4IqL&rQ>#:Ax-#%,>>#'3G>#cEJb%gWQ=/W`:5/)WS#,/D*(=fU(*4;=F]-"
  "FfffLFG_Z-;qD)NV@)aO#'pP)/GD-(])iO']xW**BnvW-$p6C/&f'B#b>wgLX3`HM_sTM'ODB+*:Af*7teEW8J4Q:$K-q)NP^b)##XMu29KWV&6(D?#97[s$Ba(v#&gM*e:0C0cO&n*%"
  "B#o>$R*Zh/-p1d>E_s-4UpxG3/NafMokK,Ns1E.3QX6thfYu`4+rbA#1HlG*lq-(QUc6=.Rq@.*Rfl#%Yh?lLFxMv7.r]G32FLB#*S-7]?E&;Qqvu7eAC=4o=bU^#ndo@u?;m]P2LJGO"
  "vQopE2B.u/0fU=-Ie%s$9T5W-qT[w9F.xVO..`S.UXb>-7G#W-Hk;c.+%6aNpn$##$&>uu5bnw#QKb&#^u)'#_LkcM8[UB#^T(v#(H(v>1bd$$0WS>#vK3D<m..<.O&ee%1%=g;lC(a4"
  "#::X%5ue20`ZD.3oe-=-%rA[K^Hl`M/g?s,1Bn$'TA*XL_Nu`M#lVAN:-S=-KwP&#Y>0,._A]nL</n##O?8W7F2G>#JE4OM:gP]5)<]Y#1m@-#wZj%=5Oow#+tZ0)7Ij?#s0Y.<9%9JF"
  "Wh?lLJ.JI*`q)i0_twl/4N?>>;1OB*8[/YGH,cB,2CDk)u^j[#q8._#_Ifv$JPjq,;(4'*JYdg)9O$W,4a?['*M;%#+V1vuurO:$x_?Y'6?n.#Y/#a7OeoJGQgn#%T%/=BC3c>-6sqV?"
  "63Q.+eT9U%D)t(5gx1?#.MC;$3Swo%?.IW$St.='6.V?#3oh;$Dg(v#pP6i$2F[,X)cl8.4A''%s*Z(#6QLdH*0]UC@dlG>7/9=/HtDH*4d>K).v$m0@*_-%<59f36+LS-Y@m<-4^dEM"
  "*Ir8.dpn--Xq.78YMMF7Q3F9.N&B.*De])cXt2.NIT.N%bb]w'@X]T%M5V?#Mq)%$6dP>uG(D?#&U,Mp/lj[X/Ofl+L]j@0T8'vH3Pn<:iGhIHXmURMlbw_%-C@c;Ih1^#d`GhLF,E?#"
  "g7kwRx<g;O:u&*#)&>uu<jMs#iUl##U?O&#2,3)#cNQ^7;r7&6pYda3cUap%W7M;$xK,+#>@ws$JYx-#3cUv#DY4.#6fLZ#X]tg3(=AiD)DP##-^XV$,]UV$:dA>#0m@-#vWj%=1_t>7"
  "uQ%0)#DF.)i5dn&Uv.b4mS6C#WV/)*k<4d;9SDmL4UkD#:ke;-qF*`=YvB#$%2K,MoCw[-:*E$<6-ge$rH,7&629^%A.MZ#/kY#u/-N=-P]d50'=La#Oa)a#HFWhW4'(lo0kE<-8$]U."
  "U%96&(G#W-Hxo>I37&5#)&>uu(ri8$:4=&#XI5+#CaX.#OMmU7dMYG>lCR9Ia3nH)YIlN'dC<T%<vI$$MXN*,m.X87]njP&/DG>#+8,##.eEXAXxN.)v3.,;&Tma<u:U-#0n$eMUZhg("
  "e,GZG5OjP&G41/#4f(v#V0GG)T+ow>Y3^M'LhsT%Pcx-#DELg(F]4.#O0^q%Xox-#7u6W$ES+.#2iLv#9&`V$5(iV$IZ`m$q6P#Ra+i.;;e^6ETJu8ETYP>#O.`F-[S]F-K/YC.&.Y0L"
  "jN<vE))gBR;]#0L1uL?#xjZ&Fp7t]7b]RC-V,Y,H#bitB)Xj2<5EpJ=`CC4;hH:B-OiZA6_Oq+*r@X.)0KCu$C:lI)$g(T/+_D.3u<h=%lK(E#==Ox-3TF:.R&M:%kVae)+W8f3%OIf3"
  "$Em;%h]DD3?DXI)nv#E3`nS6Cg93R%P`7R3CD,c43Fo8%uf0i),S>?.M?0OhCXpY.9uO2(3Ng<$^W96&?1gv5QLlu-eEH#,1<>x#p@'J)a]v@--Rcj1?CP'$]`Y9'FwgD#i&5o/QT5)<"
  "S@HQ:L^eF4Sero.8ai0(pv5v$jmtT%E>]Y,Fwm#%?/G^?7;ei1)s`4'c%>;-Y.+j1Z#5F+%iD8]_@xG2B=V?#s)BV8#Ct5&5B@EloKSo)+3S##;lB+$bsc+#bd0'#L%T*#pNQ^7&>Zt&"
  "tCto7pUap%07/s$g/>]##O[G;.c$s$4i_V$]^cY#84VZ#HVqo%;bWp%DD:8%1%@8%iGMfLM#<v>-PP##b#SfLS0eF>TA&:9&<OjL+K=:.n34t$$2Yk%4GU%6O:kiEsR]L(56+r7m(EmL"
  ":`<f%<Z>x#q<x'&qJ))31YI-%DjEt$I8#3'b:wDEedNh#JTE=&Oj`.&7L1^u0)96&ju6qLudXT%/7^gL6X`=-n/TgL0Ckw$<34W$M/4x#0R%:.%-9E<vm)gLiS.)*Am,g)ZZ&BO]=?>#"
  "$u-20'k^1pJ)[`*cl`NrkJsI3oJ:l0,Ga01o6H,*[%(],]px[#sHp;-.^;>&x1g?$*nd4]e&lP&XLwK>$m>&#=hK]7S[f[#7:r?#?EY3'>i(Z#V^o[#$F[G;)M:;$Tr7/L1i(?#0fUv#"
  "IT,eQvdR>#;/>;$3^1v>0u$W$6gh;$C_t0(qr6<.-7Vw,$J+.)]AW&+rsUO'`cfF4X?hc)s46J*78Cv-US3I)oTL,3q+Fb3`cvFY=m<P(4Xn;-4hgW$OUB;6rUg/@a:^Y,`6x5&91Vv#"
  "`5V?#-KCu.St,E4+fr#$Q54`#L3:a#W/AB$R3N[tGdoG#I9^T%'be$0W%0AXEIV?u-,oC#&%1AXKF$hcK1?uu`]5j#N0`$#n&>X7s7ns.n;)O'<](v#1/>>#MdqfLXQccM'ocW-g@9-m"
  "jD;q/E@uM(Ed`,uX;Z;%_TIg)a%NT/Ta-)*oFn8%Cr&t%*o&02cx=c4Vb7%-G`ag1hN+x#0JcghWf(I)>FDmLvj1pJSoCZ#WM;iK`Bt2'Qb)Z#x35v(NA`C&SF*6&xPml/pAB[0j't?*"
  "B4r?#$ZoX#MU#gLki'aup>dE4fb*X$;FFk'iJ*6&*0d(D7S###V(f+MmBT@m7%rr$lS2>5Tjw.:+#Pj)B)G5(0o?8%]rmC#s3IG;kGc>#u1#H&<c1N21oU;$]+8/LT(ofL/O2/#*;G##"
  "D:Q_%1Xdv$BLhGD9Ql9.E:>d&'ZW50NPU[-STUX2xSCZDAKTbO;QH##7Dn?U_8p`?(kB%#=WX^6=@#70C=q+*<GY##Lwns$mvMa4D/NF3o=km'*IH:7:o,[nmn`:8t%vG*2fOn*af6<."
  "plp;-w-We$bU<-*bU:8.2]2Q/l[]/1U&&E>Y2jc)X.g.&1>pV-$JhAI@,>^,QJA^+NQ5n&[5XZ5mCOa*/4m@Oj@,P34Xd01V2$e`ppl&-u]XSNE>`CaSrT0%eoVM'](T#,9,[S@o(lP9"
  "K'QZ%jO`uPB]cUV(CqhVvV`?>dw;;$8:$DE,.NA=NS8>,7%KV6E?:dFou:v#&^#[-ApV]+@lh;$)TO?#QtD;$CM1v#+,%s$aW31#bU0dM,&###)8>>#t*Ke$ScL;$s.JgLlXvY#co7T%"
  "pS82B+S1v#D,`$-bbr?#gG%P1GwY;/]JU$-6v.l'vJZV%9+i?#Y2[)*c<RF4<ZVO'vR7C#I,QA#GCd<-d+d<-@Bou-'u,aNGu?x6W9RYM2YvFdD;f+$jSH_S56p.cE=]b.1*;S#2-?UA"
  "4iPjL-w.62)bOoLPft/MO]XjLWB9$N7kWM0ox%@'IB+C/YN#kMS^U&4%2Puu;oB+$*'D$#L?O&#9or$-o5$hc`$[>#tp[Y#YQ<1#(,>>#;G4.#+GY>#)3G>#5dxu#::wL**veh(u]Ul0"
  "248C#+'ihL,%)u$&C&s$C]R_#0Lk]#mY*E*QiP*RaVSh#5O_Y,^a(Fa;N/%R`fB;6Q_kkPhO.29&XSfLa6,5So]@#PYxQP0&65$7OXXkPh@M58>iMXJ]9>%#(&>uu7SKe;Bi*c4,R+;?"
  "=B5F5]'a20x&_q%+;,##%/IW-<.[s$RU2@&2rqr$G7sQ&1`(Z#,`$s$?DPv#8Xjp%^08F%mc8QA6@wo%$EPG>Yfoq9isv;/W8k/M4k5Q'C(Et.e*gm0bl@d)DO1x5oF>c4R_qB#v4NT/"
  "X7@W$dt1gCjfuS/T'YW%mHuD#+^B.*uZ/I$s5D8&bp'6&[T$#Qf^_]PRp2d2ajt5&`wBx+9?h/%t`9N&72'ZnWeH1MVphc/ww-7]?9FQ/5`5B=p,p4B:5h^#Gen>#Tj=oLSX6.q[ubm1"
  "/+IhG9d4%,_?hq@0>uu#8:$DE+*pP&;KA=%`19>,5(]s$.AP>#+A>>#HKM&4S$###*=l]#rF3[$0+wP/`(`?#d%J?pU,Os-I]7C#)$fF4=Ha.3_EOA#*GFJC826B#xS-gub>RP)f#Uv'"
  "$-(SIxeKf)'8xa*%u=r)=x/S#+%cDP@'1Yub)Ns%8]4ghdaS@#i6EW%L`.vPA7;kF=Z)_Scv?4$$kWg6=(V$#MXI%#,ElX77(][-#;=.)f>Mk'S<B6&lFGb[`(ZY#$h6^#KaZ1%>hp:/"
  "3Q1#$p@3[$iN4Q/4x)?#M[lS/)ftA#ksl(-2=.Q/YF(*4#L(E#``LE4>A^Y,YPID*`.SNC--#^l,,/p7]ULAl-O,iLD)aBs1:IP)d[)U.4D1v#KrZ8'F6J@#ZBL$+]u'[,Wiob**q$Y6"
  "0HwA7E7Ddu)g<N0`R:d@KJ04EZKB)Mt%qa%W]K>#bKt7KL>Iqnxm%HDE4EW%[xdDEEKT_kckk&#<Lfr%cXNe+sPux#A7Ne+mZ<?#*MC;$#T(v#3fUv#[FQ?#:[sp%Dq'5'U7[s$R]mp."
  "20,##4+Ne+7T=:.R56<.*v$m0;IqPA4rQ#6#W<4BN&Wq0lL7N:ahK/)3<pN9B+hHkL*L]XbEmal?#sbrr*AX-Hj*:'UNhp&+:3<-8Kgo%HM&=/5Q's$c'96&Jmr+*8%+T%xQPA=Oukr-"
  "ZJp(<OlXp'>Auu#6@/s$SOH>#rG%4;:^VC?ks0@#Y=bcM2o:Z#QbHG$6Fws$l:N9)@tjp%Hv&##<=;$#ivqh(Q'9m&F[aT%L]x-#81n8%CMx-#qTY</7Vjp%dV%gLO):A=N63Z)c+B/;"
  "6#^v$d]s)[#e)N;u;3^#,%T*]bdJ]?$gC<7=Ek;7Lrpk;NY4R:>gM99.tgv7=@g>.@/cP(vFOI)C+>2(3v$m027AC#[tPm'2i(?#JN3'%P7Y'4nt9C4gMi=.kiic)]H7g)<Cr?#n1$C/"
  "e#NhL[$pU/Yc51MJF=F34;T^BUSlY#+P1v#TZjj)=beQg^s;N%<Fes$IIOE#;H(T%Ah&Q&E#X_0`,6P0r+_E@9hY@JSXaP&;V>>u$D01(p5BnL3v[T&0JcY#k#D_ugW<9%j1958=;YA6"
  "<ISA6$.d:Q=V+58U62.3`pMP8Iu7$7NFIw##ehK(,D)u$VD[+#a8DO'eRM@#j`Rh(V1>3'?IiZ#UHcY#2V5##av(v#CnRw#TIJJ(L&VK(u=<e?_]Y##[9pn&?4@s$-^Cv>6n;w$<PUW$"
  "-Su>#+piu$##jI?xS@L(5a+a-;9$1>ZTH9MqiMS1>sWs/%#r;.$N%i;tv6?.ko8f3#R_,37q_k1IS8L(S3:t%)?.90>LU58Sp^;.^GrQ&IU;78Q6Kv-#=n&6aKs?#-2B+4x&cY-(afm("
  "Eq668*_tA#MM9H4`d4t$<+R:8$QMs%g*pv.5D=/6:dC(6&&1fuf7d9M3m@9&1PlY#j[)W-Ci(C&'6Xf:0(3v6[B5T%>v1A=/fd5CRSf6:<F0Q&fg]S%ZHA8%[ndY#1gst&7@I<$qY%[#"
  "A[sp%JYx-#-,>>#TLie$d=*9%@`4.#5uH8%58w8%eY.gL19h^#8h^v7Z_V.=@BZW8]K?v.TQ=:.MmTv--v$m0u[NT/vu/+*ULNu7'gDt.lFL8.e4NT/a<7.M^$ra4#;:8.UktD#LZ>x#"
  "m>sI3NAg+4c9Gj'a]/J3V(ia2W]Dp/7d:^uABsr(d`e_'-92</xs^6&`,Rd)STmu%v2Se*Q8Z`<id9T(&T@H#7Oa5&#C-x8MMOD#+M;_,Kbe7(<2L;.A;Z',t+O87sRg58l)6b<TO[I#"
  ".O5)u<I39%/`aJ2_<O)8jBh)3Thc+#cfOP*o_cM9q1487$i't6k>hd2&c8q/84IW$,Y&6&;7iv#WT(v#>=R8%OD`f:M'[(#Gbsl&Bg1$#J-B6&PAVZ#5uH8%F&Pp75E;=.2FC4;/I4B6"
  "-Ya=.B;4I)NPDN00:=<%vu/+*]hJr8Cpt89)lFI)(K#w-]a;aG)OVs-@AgF496+r7UiwuQ)UE<-Fb](%`#`,FjG]o[XZOM0Wi^Y,ZBSpB%`S,PP[a7%>Rw8%<U<LDIeWrmGD]51TXjp%"
  "q.eS%[,nN'='j%64Q=.#$&###l,CSn1E,>>R7f2;FK#n&m`WP&xCww';rLv#+25##QVi.L1;5##bem?#;:E5&=33Z5?G''%2x_Z#i0wL<L8s99nLi`5N,B+4S,b/M5KR4(^?o;-CkA+%"
  "Le1O+nZD.3cxDC=3g1T/YSW:SHqIu-M#f;@Lpr633uGg)>)V:%nif$$JU-7]:bH,*q]6>4U&9$&dH.Q0Asb%P/GB>,EFTfLN&Pa]kg1I$utPOEP_k&'^8eGEP371,1L7DEJixe,(2###"
  "P4I8I:u3T%v9GJ(IRPfCw&YV$r1ww';XN5&dj(v#mxZ['54[S%aTG>#^D((&/;>##gf?@'2r_;$_qi*%W's;$7s?s$Ew^h(OqET%+/###r7Xe)jJrK(b>Mk'ME#3'ht@>'xGwD*.uv9."
  "4VP8.Hf*F3KO5s.HBhNCR8(ePvGR@Of5-&MRIVM#e+QV#_wn@u$1*DuF>'s?ZE$Gr$D=VZx&*h#)5n0#<B_kbY@NP&tRKj2DO-%$u0@G;(AY>#XHuY#[+h>#Bk1^#9eqA4/a.-#uH3D<"
  "2=*9%-`UV$PUIs$FwucMV4SX-L1D;$lRs#0btQ1uljso7I'EY0l>u1'S<mo7eTPfLN[Zuu,qt2$k`4oLw[a$#R&>X7Q;cY#r8IwBE:IG;,YC;$vauGHG'EP(+hE:.u]Ul0<.i?#']R_#"
  "S[M1)fnn8%Txk)3v7H>#=`s$6UB[fc$gD5aa>u7[SVkcu%>uOHUIK1gn`-;HbK#PHmthoU.Y-K#C-JoPkr>&#eqAh*9o$W$`*]w#i-:hL2uSG;*Ml>#]gh;$].?;$vg-<&:ICp7f#g*%"
  "Grdp./SXI)8QET';OLp72N]i)nY?C#&VW/2S,LB#V)ZA#Rg;E4Rq@.*eH5g1oZdm8+T1N'UAs]EHZ$gLVh)v#/:#3'jngp7<:O2_5-tp%T5tw$R$tI#MG+gLtdhZ%3]#29Xw:Yn+dTS%"
  "V`W]+S5p(<vx.7&_Z9)3E5&^+X+]fLev)?#+Pc##mk0W.5%iv#f,*Y.0](Z#5xT%11`l>#;Zqr$+0;,#?wkUA/lqr$UwZ)4/Cq/)dkns$>C)W-@54a3<59f3_>Cv-jFXI)oTL,3rFoU/"
  "0k?&%h,`CG=]?T.2NId)x@^Y,)CfP#6%$],>^GD&R0&E%BUWT%jEx5&Xl4P#77M5%I6cZ*#YG,*f=U=-s0S;mDKdF#R<IUnOlt5&Z[-T.SBv]uJ_Jsu&4UW._ko[#R3ST%BkBT%MCe(#"
  "%gs[tn>$5oFdXS%qt*;?Rh`uG*cf(NWV/ZI:<Z#7[V-TIx,x6DIXAW$SZAZ0X7b(NW#uj(dFofLOqA6&&H98%>In8%Xc^Y#>+iZ#'RVKuxRns$H]qo%1.no%<a&##(>G##</>;$<RI<$"
  "oMMfL<@CV&=u]Q&5dxu#>Z1oCG;%E?;TQo/c2pE+X'4t$Kp()3wHff1x()Y$OCr?#Bovs.j$d,*g@SfL>`)T/fxwJ1+._C4[j:9/@<Tv-X]Hb%>P,G4SXb^,X,U:%bri,<N>A=%VbNT/"
  "@Dn;%mbK+*uR,s6]v%@'MNAb.lAF1)&@%lLeOP,MKEY#-Q>dZ$aZt]#__`V$T=I`%7tm`*[juf(t]aA+X[w1(2Er?%g]V.Lq4nS%9tc12:=IO2B9ZA4q6@o1EuPm8E]Z5/qwUn1w^?)*"
  "_Uch<,rt%$s@Os6_L/O4L>]*7x/VL+-,,c4@Tg*%7-)80J?<D+#j8J30s;Q/1D&VSJno/1@:<p%)8%N4ZEYau.tk;$>nxi'Mu[aum7(s$`l=%,6<Ix-=+jW%(?=Gi+l:D+@n,/2C#fT0"
  "8fk,2kn;m/jK1K(?/'r0n@E11?5B71,+.p.)=qS.WDld;n+QY-a-9b,*8O/,eb[X-*1_B#,h.H)ZJ7P'TVHm3OWko7XZj,47.@W$_j0K.Y,oY#qx8;-&MEf%]VUV$XcL]=-G*B#,&$`,"
  "/fUv#GD`s-.PcY#3i(v#*Nww%8Fn8%XHuY#aF6?#0AG>#<4u+D[iIfLB)doLNIZY#4qTw07;?.4t&=L)?oOI)8>/1()v$m0.?_q%>+kv8Y%lk%ZVd8/''A/&f-Qd29)b.3^nr?#Z]B.*"
  "S)BG;C&p-$9F(,Mh2PB&SF*6&Ra'6&:>-EN5]uCjwC#R&Gx[t(w8^$0Afq-$uva'/CXlA#DL$iM>`?f$wH?&O+'2HMMnQ:v$UbcV7%rr$MP8>,(:)H4cEOY$#,]fL-wm>#@d:Z#+GG##"
  "8R(E31m.-#%[WD<YXu(+^HB6&9`uY#+b`w$r>(l08n@X-.30r.D19Z-7BFR8p?u`40NCm/=.sx=wZ<w#^ACM7&Y%0:8*d+)^M``3_j33/9;x+MomXm&RobIu[u_rZ/xlgLuYPgLfd]bM"
  "PDW$#r>$(#K_0T%_EQiKRrd+VUJj>IN<3=Bx3L=%qctJG-o,M(2^UV?scEs%4Y(?#sK,+#TXrZ#I6tt$Uc%[#B:MZ#1iCZ#ODVZ#6l(Z#'8>##ZW.$pl$8@#4.g,2@A/-#B^,F<2`tGR"
  "E7%:MVB,Q/?dlG>5)0=/HF5<.2v$m0Cl?(dn1Jd)4(XD#u?lD#:@$Z$]OfA,><S>-rN]$QiH`ENSEC&QlttONVm<P(cBrV?1P0_,E^?K%@O3t$tY)BOoCB>,xUjX(iu0MS4xSfLaWf>M"
  "VD#W-ji$V2F6a8&k#=s7uHSD=5@*<-NJ1d%/4<<-JAJ>-.JK>-:X/r/M<*+/^]V^#*#C<&+:(d#FNb&#_CEg7%$#p%jHE;$]<]S%tk4M(7.eS%pZ0[[(lj]7vksl&IH#n&Ja($#Chsp%"
  "A^C;$5+nS%<Dx-#8fLZ#V$8='.55##U,(5MW[6##/-,##:I&f+YB;L5b0gQ&jT4/3T5`V.liKS.up?q9llpf3#dFjL>M2N(q]@L(e<Ot$PB[g)Ge75/;8sk':Cr?#;>#c4.c%T%%N.)*"
  "8=n=7;#G:.W,ic)r^v)4$,A&4QIVs-_k2Q/%cd:AIO/A$>[xC#aX_;',>P>#6sCZ>&7W7D1qBOMjf)[$T3<A9I*2GV^D0v#uWuR0AkV@,6D+w5Z2O_5H4k11Isux#<UrGPdI>6ssSCQ&"
  "h]Y2W]n7n*94Is$YtU#Sdtg,Hq*RT.kHNd*5+2%,Omc/(Kb<U.F_qc3H6f=-k7&8@(S8]?WLb`*_cP`<i^X?#t3@G;F/>>#I#ffLl(9s$Vg-s$-JP>#TW_V$Xc`MK,fh;$Wl7/L0oqr$"
  ",Cu_$fa:v>1uh;$6j$W$sjOt.H((+*iqes$HZp+%(lEp.?;gF4jG1I$wNf@##5)I;2kha4[q'E#W)aF3k6'Q/N06d3Z@AC#e05/*pw6Zu2[TJ)B*J@#V<(c)'I+,2?_KZ8^:I0Yr8*hY"
  "iaW5]_M``3dmp5&Y@k(NY?/<AFL&R&g6PH)McnO'1XjY.@;x[#EsCe2>W:^8I+BQ&FnW9%1]7Y.mKCe6T.eS%8e@U.,$###uFt$%?#wT%m8pu,$Z:D3@4ku5]V5D<5jGfUf$hv.H&5L:"
  "5ND,)Yk=C-^#,A#-(ui($Uu]#$D5?,T'2,)d'Aw#PTpq%q^2o&TS[L(6oh;$VTBR)@lU;$Cr:v#)Dc>#OG`Z#_tq%,]9]b%,%ux+Ixw%#rbq.GtmRqCJYwA@gILO;*IO#7hIi`5WV,G4"
  "B<712dJUv-LNo],<[2D+,iK+*rueh(8xhl/%rPv,#'A+4B8T;-Qf#<-5:#-M9xu8.aT^:/qx4w$jL0+*K)TF4'sOk4r;sk+Q0rk+6P=jK]levnZt2.Nt7Jd);m#l94PlI](**f$NwWa]"
  "i(lG]jQUtV&i]5cnV^TpW$E?#)>'40O%xw(`Mv;%cdJe$f&(6&@;B%u.LoQ070Cu.x]hkhq]Qs.j.jE,59RtVNdeG]DNnNu^<OFM(qW8]RjGI%@O3t$QK*^-;HMk+IHBUEda[21$%Vt:"
  "7NqS,2'Rd3873iM2r3R0&,###$umlS6Rux+fkAS@$13H+m4RW$o[7##G[iZ#%ORG;ZX+p%#,I8%r?`5%Ekf(#817<$F/B>#-2>>#HTl##6(iv#@Zu##3ou##;<G##6([8%@g-s$/BM,#"
  "9fS2B1x-s$^G_)5Zg`#$59H9M2l)?#2MBl5?Dr]7OidM*@9WH*,ctI)(v$m0W:AC#4/+E*c?XA#g=Us--K?C#jgBW-P@$1,>`E=(0Ulj1p:gF4LW8x,t.K;-e:Tg%/n>V/ji4[9gE(E#"
  ")kMk+xY*E*,6(<--%a<H2+c**5`cY#pj>N'_`E.)8qxK1?vbZu-CC;(7%JT%3B1v''RFx#g_PG*L09Q&Kphg(FbBF=%7Y$H9<RL2(nC82ZHf-Nh3kN0j3V<H_N=x#v'(hLq7Y5&'mLh1"
  "pdGk'%Ni+VxDC;(kX[01%xBk=,IBq%_XD;$<I3p%xWDuPkCZ42(a,IN8x_kLScwt1G9%@HmVD@'35^l8u$]v5b$oP&IPou,`/;W/-85##'/,##Rc^Y#X+?;$-dhV?,Pc>#4NC;$VAj/)"
  "Lu/1(wc_l0lG8f3p#ID*kpBQ/a+-J*K;gF4JWGm/I$o%>E1J>#XX.e%f_lceP/ChL#l(rEZVd`<kidCa#P>;-SLMmLh$mcEoPCvCY2?>#sGW@tTZk]=,Mju5JF<;$1fh;$V=2s$rbTg1"
  "RG;MKT+4GM0x-W$-Kxu#7&A-#,'B&=dQ<#-(;4.)Pens$k$;8.r>(l0#W<4B0]lS//%9^,CB-t/4WU-/x(0f),eQ0P2;;K)%VO**'E>)X.NQ:Vr+v>D_ElrDliiO'B2i(6.4Puu<=$,$"
  "A32,#j2h'#=+0h(n0g*#pp<v0P[0[KjD:u$gRtm';Cr?#*f@C#R'85/3#>c4W#wA4`4n8%R?XA#%`rS%M3,x6AZ$H)m)MZ7V?UX'Z[^H)Q87H)L8%P'K?%5Esnwi(WOgV%5]1Z#>^86("
  "lunH)dH[EHhSK6&`0Z4,IKP/(w@PD+uR`R9X;TI)N%T-EX+Pq&Y3t9%$r'r&K3^M'B86g:6:8=%'i,P]6HkG4V.%W$*_&:BJQeQ0%/>>#[^Lv#B?Ok3TTL;$U`VMK6fL;$V]'##+?cu>"
  "'(i5/06>##Gxk4BZA]r2UGcY#4o-s$Zv$s$bxL@-eZM3Uek)[?-KVU%6g*]%UlXn:bZVS0fP4]%nxqB#5fWF3_cDB#)>x]44*fI*mSID*3^G,*E``h)wH<u-#<Qv$gNCa#s3q+M<c$a/"
  "Yo[_#^fjI)c?XA#Fb(f)1^c8/@e*Q/uY3aA)GkS&G;%A4rve&,54cGM+*%ZGRF8r%w3bT%2d9<$-S$^P#dAt)&a^s$=+)Z#Ol's-]*9i'9Sq<-fjo[#t/wiL_he[#iOi_t_')>PXfv?$"
  "QJaa*8B`G#c:rbral^1)4[_u$U,@)*<%`Z#N/Na*G,u?0_VlY#:C<p%Bx=D#MaCn&r*T*@.'nd)En3t$-D>>#7=-DE+1sP&lBel/M3#A#N,28&,FwM0uJ1v#X%$v#CEr'8^tZ(#=VjC4"
  "xX@s$*s.)*T1h8.[P[]4aEGw$E1tD#kJ,1MhZP'$KjE.3#f)t$b[s/(@7`Z#&_pV$A:%w#Vr&U%KW.[#iE9Z#QglR&b)UZ'?*9U%wvBD#@=`?#gMPN'6MYYuZ*7##2J&mA[aZp.)Zu##"
  "TBV,#Nl(b7TIJ>#d3aV$ddA8%Q&Ma#>JxH#0OeG;F^tgliY:Z#NP'##7u68%Wg1$#I3>3'CWG>#/`u>#jMMfL)3gs$FSUS%>bfM'BP4.#SQPN'ALuN'e/MfLj/sG<]OXqC:Wcc>Zfx6:"
  "%(]A6RsE.3jmJA,-',01+ooh(bWeLC<H1a4l5TKuE52#$%9tWoaP8f38D[V8U9-$$Io>K)tmlA>2j:(&/uGg)3.Wa4')'J3D'[mse/Gx#1/@i$(A3K(0qwX-wX#s6JX)d#^fe.+<1Pi("
  "]k7/1,H;./(vfB+u[M;$ddcn&x[B+*vhYZ,c'8t$KKu5&,xl8.)ZDr?kEJs-Rp?1MR[+&'Z8_PMf$L]XhdoSodI=;?Bf%T%76F=-lr;117*$v.,KiS0X[nw#<(r;$?->/('YMG%-3n<&"
  "RCw5&>:wDE+qs5K+IBu.:`Ue$@;m3=B%j*I9#-&Mw+]p&&2P:v@ZK8INV2/(%p6A4Ya>Z.+GP>#)DP##<<G>#0ZLV?Ag&E#5g=;$YRve4kTWH*`VS#,]U5l1WE/[#4N1)3aKEjL-PrB#"
  "Q.i?#CLA['`lWO':>Cv-?`8a#woq_,.5Zt1d465E*d6+%*nCA1Ce5MiVdkf:SQblLBq*b5N`XauN)b$BK)cY@Kq<VZ(ikA#T*X(O;.w1Mt`2:SqZlkL?K15:GwV&$*HPBS>I6`#@c-1#"
  "B=Is-pH9sL8>0%#^doW7J####+Duu#>HcY#+WUV?&rB^#[/MfLsaJ1*3@hf)f6Ma4%0<a*'m>x6/rfKY.rQw^#5xfL?tn$$uooY,sRgsMUWgsMR4dJ(bBV_#'f2_I^7g?$cH*J#`alT`"
  "8gIvQDh)a#0vdlgJp6K:&,###:#WYY8'1,)VV1A=GKU)G=O.w#i4//L,i68%>LU,D4&###4iqr$N>MZ#6iC;$HC:,DOkE5&H*#n&[KFU();G>#[qi*%90L0c_wZ>#L,Ui)2F3D<L8/U9"
  "&==^6v^X+5[$VO1lrQW.FwMD+EdJi)+RBq%=I@@#p9UdkpI]L([1.'&9[XD#.Of@#)W=P(nEZd3M)'f3_>Cv-ouUv-3`NF30k'u$=S4I)o1$C/#97<.ubLs-fZ.L,f6ei9ca?T.&i:Z@"
  "NBX:Qr$LS@Gih?6t>x.*tnLK&uGa2gvqoUds.%$*l:,X6.H&Jh8X-g)IB:_,kCu[,nPx-D9mcA#YY;^7GXl'8@PTp7;#3Bu>,-e3o^71(XvOA#lw-g)lj[h(x^Us)N`KW05Z>A#4+f],"
  "CX_iLlL4RMA8$##Z7CL3K:?_%NA<A+sp&/1AIgr6i=M]=A`K`Et7JcMp<wMMUZWI)Q5,Q1si;B4bCrV$wK,+#fYje)cxx-#oPDO'PAnd)lAR.)PT:0(>rU;$KhqcD@xhV$-55##==LcD"
  "Af:v#UZo@#g+JT%rhlRATJe`*JuA$KJ.&/GuZ`tB;jC)?j_qO;<BH<8mIVD5HQeL2gWnK);.720uXPv,kO]:/RF)qiT`>lL`Ej?#JESP/27:^#O`s=(uc]Y,VP&5+WS(4+a<7f3`4n8%"
  "8%9U)W:Xk4]P1a4@vpO93IZ;%5FN/)tNs;-cR>1%('KYTU5Se0a69/1?3q?NYgr7[^c<JiZn_B#](VS0`2%51-IfN1p^D?u>DB+*ct?)(foLR<*LZh<fJ)g(9($?,^XV=-GlYs-;(UKM"
  "s@/nPxRo^NORlDNt5k=GC7._#Kj^e$>>v=Nlv_5QMi<I$+.gX(rt60ul%Z%.v8(pLK7$##Wd/.$aqdwIOge<6+FRw#oZ@U.[9Jl6kH7Q/31-DE@+jY>4h2W%WgJe$c,Fk4Jt((v=QUV$"
  "AE4v-_]K[M;beFN`QW2M=fAGVV9F,2V?O,2*Z_V?&Y'#vc-:hL;fjfLpN9`OdR#-M'mk0MrZ*8v1&Q5MW9;.MO5SRMVNx>-BxR/Mk<oY-LpNw9Pov--)'_RE3Ih>$>ZKwL6*>SnU=XGM"
  "O21[-$m#x0_2Zw9w$n-6$oK^#R6YY#xeK#$U;_W.nV=uuFmbZ-^mTEn`]Z]Y85J&#+bDmJ/_sh#l1_+$RL-,$/8=E$aPMo$hm[H%`1?$&_x&6&(RxT&(SVo&EN^:'-#j@(`dq;(>q@p("
  ",)jD)hr``)1]qv)kTHO*GuMd*G@a1+0:0B+Oido+grq(,oZ5Z,I@O6-g0m_-`$.d-R+:v-JW>Z.OaNa.ZlsA/w(rH-#YlS.V-j<#>o8gLCQ_l8/F$>P[7,)kp/Ke$4QTj(XvE>#4ij-$"
  "SDl-$.(1B#app%4<UL^#h`IV6-xB#$w<,p8.+_>$saZJ;3LHZ$-<gx=0=?v$<1aSA8$&9&aHm+D?QxT&t/SrQ'5>>#pGa9DU8[%X/(1B#C6pi'01L^#Qlxu,-xB#$`meM1.+_>$d.nY6"
  "3LHZ$2$]c;0=?v$@UxlB8$&9&C=iuGPchV&ja5Vdk^T%J$'4RD#*ZT8$eZL2aJ^PBYf=rLG/=LFsCD5B:8QhFj.nV72.vLF+^#lE>FwgFIV@6/c:@bH@j8PN?b3N0pkn+H5-xF-:@3IN"
  "`/hQCvs.>B:RL6MB8r(IP;DGH-I8eGgRfP9Pi8@-5/_4E9V*s1:4//GGbAbOaxCP8XhdxFeBEYGIYo+DUw%;HlBel/YBM50Z8#-30#v(37ud'&I1He-e]u?0^/RgDH2w1B)J8R*rk@VH"
  "`&VJDTA+#HkUjV7/4tiCAEMDF@bXMCSpmcEV^<j1d6Ne$A%ZuPY#e+#G#>G-aX4?-jWx,QTOm<-MW?X._/:/#=*J^.uJ6(#VnE0Q2t&nL[qkRMRxIQM]Y'kNuA;/#TaoF-cLM=-o_nI-"
  ">5T;-'kg,.n#daPlOG&#2/Uk+xKRw9X^p921[2GDr=T&G7&$,2TAs.HYJs&#%/5##(@V8&Dtuf$KL4E#*AY>#.Ml>#2Y(?#6f:?#:rL?#>(`?#B4r?#F@.@#JL@@#NXR@#Ree@#Vqw@#"
  "Z'4A#_3FA#c?XA#gKkA#kW'B#od9B#spKB#w&_B#%3qB#)?-C#-K?C#1WQC#5ddC#9pvC#=&3D#A2ED#E>WD#IJjD#MV&E#PYs)#lq>lEki`]&`89O1,gpKFa:b7DdL^kEo$9I$CZh?H"
  "%#%d$+oR+HjODlE)#@&F9C4,HZ+0jBksaNEtk'oD#c5#HRA_<.ij,@&>'HgDjH7UC9F76/n)fQDCH7nM5Q]aH;^<oMF9(sLp*:oM3OwgF_G6&FtrdD%D?;eE%s?)%c0IN1-V=>Bw#ew'"
  "+*9I$OsfaH+;iEHIvha3,9sXBpg`9C.$8fG=rhjEUd5/G22KH7N,L#G/.bcHhpXGDjH+F$v:7FHf_CEHgP(*Hh(LVCgZ7[$4,)*HiRnaHtj@,Mt]G>'%Zd2C/;Wx&FWiDF/h:qL0[3'O"
  "Td[+NbBusLKB.eMWU>-M=5HvG)jT0Fb,TjB_m_?T'GcdG4(29.iKd<BP#A&G73aU)@r>LF`Do=B#-C_%,/r*H>0W$Hm?ViFn%vgF6PdLF*^qZ^=/?[&7g4VC42vdGw`$d$%BOnD_TM<B"
  "sv[:C(-sfLO@`F$#<bNEAH(a3@S4VC7rhjElDXUCw.;hFr;,gC7>Os5[,V9C7rw+H<+;iF5O'A&+0o6D<Q7bF+T'ENr#t@&E;+gLI4pfLL]5t9M_tA#l;Y^ZBqC*NVAj`[vKRV[lQG;9"
  "^A@/;;V>'=#K2^=f*Es3qMaZ?\?OJDO.gIm#lB?h%9fSiB''*h+lWg5#q@HP8=#%6#sqdsAD0-Q#+^+pAj9Hm#b;_;.%Bd2$23eZ]O38%#mkm>)G,nvA-Q[G)*g`,/r%Im#Sm&H-0`3jL"
  "ON+9B7$$a<R]rY#hB#[.w<a-,X0F7/O,S@#$'oiLcfg88Rf1,)cTQ<.'$VC?U*'<*PxiQ013P/1^j`W[WL35M'&i;*P8PI-Vj=2CZ<)u<XDdv?bo92'TeU<9xHGB?sB3p.-^hD?.+ba+"
  ",(TV[XtYt(RG.g+Z*j1M7]B]?)e=vM/9bJ-g*`*N#.RV[XulW-[?)@0fg]C?TNJDO5CTV[67&vAwdY>-A<W8&[>YD4qhnF4X3Ya?Lb&V[]%X)'%vi5#)Ti5#>0u?-NCi9.X;Cm,f3Y^,"
  "C0fB(pM?X(t':k$84r`?HE[][W8p`?&=$5..#/qL0+h88`TDj$*FTB-_(o5BYL6=%kaQ][@S2Z[Ve[d+p[j6308vW[.j+huDQg?-M1)NB%Cr0,&T-a+EQZe$]F^lA,MFj$c?i0,5:IDO"
  "gG'REB4g[$#RW5/(Egj<K]1`?o&&a-8C4g7m2Ok+3uZV[J#2]04El/1KYmp0o'to77>vV%g>A>.Y1RV[?3x8BIkwpC>a^^[<U'K%4#],&Kc0eZSbapBBR8MBOv^E[?MZa?Jb/pA:jr21"
  "7W101B?M$#nSJT0r/RY%iMc98O#8p&gvvA#an5R*3U'rA6rqF.)WoE-G/WC?f+^]+3/Y-?Wj@iLpDJ+/uI,<.[5K[B%Zf,7FSBjLJ:jfN4..]-^tG$gkP;n&CeimLQN(;.>X&[-<v_01"
  "tQ5293Y)w8=&i01]2[?-FE&R8oaT`<*#DT.=<Z][9uY<-%sY<-:.;t-?6@hLT_vNB2M/?-C[SiB87BQ/3QesAN)NE-J+g886uP#)Gs'D?,IAw8_,tD'QN1W1_dW0;l6Ne=Nr.a+SX`=-"
  "d[Y=6k)q5#&a0D?<&<R8#rD?8j+wZ?sF:Z[>cMv[/>^;-3`uS.d>N:.hUl`1)d7Q#xiD^[oV(HOsrW*IL&$[-1cmIND/=fMqoQ=.]jL^[<chY?,s.m9%gIm9x:$p7t4b>-A1,])tiq5#"
  "HAGp8tI:Z[ko?Q#ttdsAX:Mv)VOo5#m`'D?K.g88GC:a'F1uD'WF^lAH?Ws^Tr,##G@%%#uY)HOi`1Z)E?7C#RQdU%aXWx0<:mS1P(:-mcRtN1t`/t['lu8.q;Qm#?D`)+2C1,M>ci11"
  "d15F%?k/t[+lu8.#ai/%CiwA,6[U,MB%821h=5F%Cw/t[0u:T.iGu>-Dk1e$ik>mA>rTA.#3-Q#3h'Z-_eL21UVH,*Y8JAGh&fM1f<Q8/ZAf]G`$1?-e&S&(GHnP.A,)<^T(cEn%^%Z-"
  "qS`>-sf@v-fr<F%t)#kkG@pV.fr<F%v/#kkIRP8/fr<F%x5#kkKe1p/fr<F%$<#kkMwhP0fr<F%%/r?-rLglA]6-41mDKs*`U>?.q05T8]xn5B=]fX[uUoM16MkW[^uf>2]7Gv2,s(m9"
  "i.r>*]&6-5FqkL>_gCs.;7mS1*A%altkBa'be3Z[rFFv2Zbr>#[daP2<0+jL-Y2u7[c:H.r@r?-b0#O-X0^kMj6(t8-]dV[c6;WJJw[:.CN56#hwkp8ovGu[FF$.M?dh41n]-na%?ZqA"
  "#'*9Bqn?p4&XV=6.(d1Vxg,Q#%UTt1oM.51BG$)*bf-@->u68.==*p.t2Gx9h:_5Boq+h+O`.Q#tmn55NYWI)f3<m5lS*)*>(#C/wF)W[L]5C/f-A$#e-WQ8m5Hu1ndR>6No,D-33YZ-"
  "t4m$0t5`/%=[fX1=eE)BG'TV[l)NY]mnc;-rZW5/35R>8Y1RV[q9IDO)D(XLhB+tAa)OW8f:JpAvmoi04bpG2/e09C?_vP8<QYC?'3*9BE4`T.;se2$+)`@-uc7'0wR5<.t9Xc+JmVE-"
  "Vb0)Ms.XvLA&3qLOlji0hq_H-A&lC.`+o5B88BM>'ah5BsH<p.N:G#$9iYQ:A.YC.AFsM6FiQp.4(RV[E,JW&X/o>6wN8j6D)nV[k^NmL#nY?.%d+c-EMOk+Zg#<8huQ/8lcMW[S)+p1"
  "o+;Z-JEGO-AqYhLKr<c%wTNp.`mdM:t<wp7S_XvI.]FB.9(7W-9d2nWAOH4OZ%x6W$UC^[DfO*QA(CP%-+o<.:BtmLdMWZ[&*si<BPUD?]<3p1pqMG)nsO,8A/gD=8i5M,/>5M,l='S<"
  "KE6T8fpvo0LIYq8gv)p0*fR>6?`f88T:c^0Be)Q8`FZkLZ%KNMdN7p.?cfs7'%f5BErf886uHs-OsVmLgrWA-3Fa.N;.O61^T6p.,T*)*TUU@-5vfa1=)pDcAtGm#s-kpL1?T>.c^b>-"
  "hNUX%E]72Nd$X61:^po0hY#<-t7V9.#T_;.^IC)N<>gS%nAlW[sRV/s^2$6#NYWI)Kh$T4voD#;DW30;WCD>#2u>p48gwJ;g'WlLoA^nL'?B49`f-@-#[a5Bt2)H2X)xG&1uAv;&GxV<"
  "W=)##jS,<-tS,<-(r(9.T9?Q#+TSD=CHNX(3p3O+dh%nL=mGoL%+p+M#sPoLH:)=-32#HM31?81,%###-R+?-'Z5<-*%29.^?HQ#6M-v?Gd8:)>M0L,42KP8IFeV@%`bA#.J)s@[J9:)"
  "-Lf'&;%aSA1,S&#1_=?-.T,<-%V,<-K,JRM9lrpL6:)=-Du@Y-gI(F.2UJb%%0Pb%EP4O+?_tiCCHNX(rx:_S.GCb.s%;_S9bpfDwTq'&u+;_S;tPGEwTq'&w1;_S=02)FwTq'&#8;_S"
  "CZ7aF=Y4;6Me.<-F,vW-JInBADKGC&gR*C&AimW[8),C/e^SmLJ,>;1=m,p0`iF>HY@/F.a3<F.fk/F.#c'I6_gYL>Ls:@'rb_VIjO/s71X%sIeNZt12'Sw9X9BO=NjGC&dRvoJV%3I-"
  "o2Yw0Y3bR<S2;@'AQtlKeNZt1S#HC&c)V*@+i+L5VwER<U)HC&k<4/MV%3I-vGYw0Z6bR<ZG;@'H;2,NjO/s7<#MGNeNZt12(,L5W$FR<]>HC&r&HDOV%3I-Y[n=19buu#maQ][Pv:N$"
  "q=GB?NTFY-CDgp0G,q^[iZi[IqcMg1Ti]6_A5pC?f2DYP&Lbc;_qw&GtNg58TKDM0/UV8&?GkM_.1NP&Gw((&en%v#oS_v[x7@4=J3:gN1Gi9.P2])0;J=2_l+<=(;8Y^?Er#0MwIFh%"
  "SNC;$NbE->l4Qn*u@q0#ndPM']XMuu@LJ>Pq<^l8FqT'%[cn^$^F$hW;%###";

static void *
res_default_fnt(int *data_siz, struct sys *s, struct arena *a, struct arena *tmp) {
  return res_unpack(data_siz, res__default_fnt, s, a, tmp);
}
static void *
res_ico_fnt(int *data_siz, struct sys *s, struct arena *a, struct arena *tmp) {
  return res_unpack(data_siz, res__ico_fnt, s, a, tmp);
}
// clang-format on

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

struct res__bake_cfg {
  const void *txt_ttf_fnt;
  float txt_ttf_pnt_siz;
  const void *ico_ttf_fnt;
  float ico_ttf_pnt_siz;
};
static int
res__bake_fnt(struct res_fnt *fnt, const struct res__bake_cfg *cfg,
              struct sys *s, struct arena *tmp) {
  int w = 256;
  int h = 512;

retry:;
  struct arena_scope scp;
  arena_scope_push(&scp, tmp);
  unsigned char *img = arena_alloc(tmp, s, w * h);
  {
    fnt_pack_context pc;
    fnt_PackBegin(&pc, img, w, h, 0, 1, 0);
    int ok0 = fnt_PackFontRange(&pc, cfg->txt_ttf_fnt, 0, cfg->txt_ttf_pnt_siz, 0, 0xff, fnt->glyphs);
    int ok1 = fnt_PackFontRange(&pc, cfg->ico_ttf_fnt, 0, cfg->ico_ttf_pnt_siz, 256, 32, fnt->glyphs);
    int ok2 = fnt_PackFontRange(&pc, cfg->ico_ttf_fnt, 0, cfg->ico_ttf_pnt_siz, 288, 31, fnt->glyphs + 128);
    fnt_PackEnd(&pc);

    if (!ok0 || !ok1 || !ok2) {
      w *= 2, h *= 2;
      arena_scope_pop(&scp, tmp, s);
      goto retry;
    }
  }
  fnt->texid = s->gfx.tex.load(s, GFX_PIX_FMT_R8, img, w, h);
  arena_scope_pop(&scp, tmp, s);
  return 0;
}

/* ---------------------------------------------------------------------------
 *                                Cache
 * ---------------------------------------------------------------------------
 */
static int*
res__run_cache_slot(struct res_run_cache *c, unsigned long long h) {
  int slot = casti(h & castull(c->hmsk));
  assert(slot < c->hcnt);
  return &c->htbl[slot];
}
static struct res_fnt_run*
res__run_cache_get(struct res_run_cache *c, int i) {
  assert(i < c->run_cnt);
  return &c->runs[i];
}
static struct res_fnt_run*
res__run_cache_sen(struct res_run_cache *c) {
  return c->runs;
}
#ifdef DEBUG_MODE
static void
res__run_cache_val_lru(struct res_run_cache *c, int expct_cnt_chng) {
  int i, run_cnt = 0;
  struct res_fnt_run *sen = res__run_cache_sen(c);
  int last_ordering = sen->ordering;
  for (i = sen->lru_nxt; i != 0; ) {
    struct res_fnt_run *run = res__run_cache_get(c, i);
    assert(run->ordering < last_ordering);
    last_ordering = run->ordering;
    i = run->lru_nxt;
    run_cnt++;
  }
  if((c->last_lru_cnt + expct_cnt_chng) != run_cnt) {
    assert(0);
  }
  c->last_lru_cnt = run_cnt;
}
#else
#define res__run_cache_val_lru(...)
#endif
static void
res__run_cache_recycle_lru(struct res_run_cache *c) {
  struct res_fnt_run *sen = res__run_cache_sen(c);
  assert(sen->lru_prv);

  int idx = sen->lru_prv;
  struct res_fnt_run *run = res__run_cache_get(c, idx);
  struct res_fnt_run *prv = res__run_cache_get(c, run->lru_prv);
  prv->lru_nxt = 0;
  sen->lru_prv = run->lru_prv;
  res__run_cache_val_lru(c, -1);

  /* find location of entry in hash chain */
  int *nxt_idx = res__run_cache_slot(c, run->hash);
  while (*nxt_idx != idx) {
    assert(*nxt_idx);
    struct res_fnt_run *nxt_run = res__run_cache_get(c, *nxt_idx);
    nxt_idx = &nxt_run->nxt;
  }
  /* remove lru run from hash chain and place into free chain */
  assert(*nxt_idx == idx);
  *nxt_idx = run->nxt;
  run->nxt = sen->nxt;
  sen->nxt = idx;
  c->stats.recycle_cnt++;
}
static int
res__run_cache_free_entry(struct res_run_cache *c) {
  struct res_fnt_run *sen = res__run_cache_sen(c);
  if (!sen->nxt) {
    res__run_cache_recycle_lru(c);
  }
  int ret = sen->nxt;
  assert(ret);

  struct res_fnt_run *run = res__run_cache_get(c, ret);
  sen->nxt = run->nxt;
  run->nxt = 0;

  assert(run);
  assert(run != sen);
  assert(run->nxt == 0);
  return ret;
}
struct res_run_cache_tbl_fnd_res {
  struct res_fnt_run *run;
  int *slot;
  int idx;
};
static struct res_run_cache_tbl_fnd_res
res__run_cache_tbl_fnd(struct res_run_cache *c, unsigned long long hash) {
  struct res_run_cache_tbl_fnd_res ret = {0};
  ret.slot = res__run_cache_slot(c, hash);
  ret.idx = *ret.slot;
  while (ret.idx) {
    struct res_fnt_run *it = res__run_cache_get(c, ret.idx);
    if (it->hash == hash) {
      ret.run = it;
      break;
    }
    ret.idx = it->nxt;
  }
  return ret;
}
struct res_run_cache_fnd_res {
  int is_new;
  struct res_fnt_run *run;
};
static struct res_run_cache_fnd_res
res_run_cache_fnd(struct res_run_cache *c, unsigned long long h) {
  struct res_run_cache_fnd_res ret = {0};
  struct res_run_cache_tbl_fnd_res fnd = res__run_cache_tbl_fnd(c, h);
  if (fnd.run) {
    struct res_fnt_run *prv = res__run_cache_get(c, fnd.run->lru_prv);
    struct res_fnt_run *nxt = res__run_cache_get(c, fnd.run->lru_nxt);
    prv->lru_nxt = fnd.run->lru_nxt;
    nxt->lru_prv = fnd.run->lru_prv;
    res__run_cache_val_lru(c, -1);
    c->stats.hit_cnt++;
  } else {
    fnd.idx = res__run_cache_free_entry(c);
    assert(fnd.idx);
    fnd.run = res__run_cache_get(c, fnd.idx);
    fnd.run->nxt = *fnd.slot;
    fnd.run->hash = h;
    *fnd.slot = fnd.idx;
    c->stats.miss_cnt++;
    ret.is_new = 1;
  }
  struct res_fnt_run *sen = res__run_cache_sen(c);
  assert(fnd.run != sen);
  fnd.run->lru_nxt = sen->lru_nxt;
  fnd.run->lru_prv = 0;

  struct res_fnt_run *lru_nxt = res__run_cache_get(c, sen->lru_nxt);
  lru_nxt->lru_prv = fnd.idx;
  sen->lru_nxt = fnd.idx;
#ifdef DEBUG_MODE
  fnd.run->ordering = sen->ordering++;
  res__run_cache_val_lru(c, 1);
#endif
  ret.run = fnd.run;
  return ret;
}
static void
res_run_cache_init(struct res_run_cache *c, struct sys *s,
                   const struct res_args *args) {
  assert(ispow2(args->hash_cnt));
  c->hcnt = args->hash_cnt;
  c->hmsk = c->hcnt - 1;
  c->run_cnt = args->run_cnt;
  c->htbl = arena_arr(s->mem.arena, s, int, c->hcnt);
  c->runs = arena_arr(s->mem.arena, s, struct res_fnt_run, c->run_cnt);
  for loop(i, args->run_cnt) {
    struct res_fnt_run *run = res__run_cache_get(c, i);
    run->nxt = ((i + 1) < args->run_cnt) ? run->nxt = i + 1 : 0;
  }
}

/* ---------------------------------------------------------------------------
 *                                Text
 * ---------------------------------------------------------------------------
 */
static void
res_fnt_fill_run(struct res *r, struct res_fnt_run *run, struct str txt) {
  int n = 0, ext = 0;
  struct res_fnt *fnt = &r->fnt;
  run->len = 0;

  unsigned rune = 0;
  for utf_loop(&rune, it, rest, txt) {
    assert(run->len < RES_FNT_MAX_RUN);
    rune = rune >= RES_GLYPH_SLOTS ? '?': rune;
    fnt_packedchar *g = &fnt->glyphs[rune & 0xFF];
    n += it.len;

    assert(g->x1 >= g->x0 && g->x1 - g->x0 < UCHAR_MAX);
    assert(g->y1 >= g->y0 && g->y1 - g->y0 < UCHAR_MAX);
    assert(g->xoff >= SCHAR_MIN && g->xoff <= SCHAR_MAX);
    assert(g->yoff >= SCHAR_MIN && g->yoff <= SCHAR_MAX);

    run->off[run->len] = cast(unsigned char, n);
    ext += math_ceili(g->xadvance);
    run->ext[run->len * 2 + 0] = cast(unsigned char, g->x1 - g->x0);
    run->ext[run->len * 2 + 1] = cast(unsigned char, g->y1 - g->y0);
    run->coord[run->len * 2 + 0] = g->x0;
    run->coord[run->len * 2 + 1] = g->y0;
    run->pad[run->len * 2 + 0] = cast(signed char, math_roundi(g->xoff));
    run->pad[run->len * 2 + 1] = cast(signed char, math_roundi(g->yoff));

    run->adv[run->len++] = cast(unsigned short, ext);
    if (run->len >= RES_FNT_MAX_RUN || rune == ' ') {
      break;
    }
  }
}
static void
res_fnt_ext(int *ext, struct res *r, struct str txt) {
  assert(ext);
  assert(r);

  ext[0] = 0;
  ext[1] = r->fnt.txt_height;
  if (!txt.len) {
    return;
  }
  for str_tok(it, _, txt, strv(" ")) {
    unsigned long long h = FNV1A64_HASH_INITIAL;
    int n = div_round_up(it.len, 16);
    struct str blk = it;
    for loop(i,n) {
      struct str seg = str_lhs(blk, 16);
      h = str__hash(seg, h);

      struct res_run_cache_fnd_res ret = res_run_cache_fnd(&r->run_cache, h);
      struct res_fnt_run *run = ret.run;
      if (ret.is_new) {
        res_fnt_fill_run(r, run, seg);
      }
      ext[0] += run->adv[run->len-1];
      blk = str_cut_lhs(&blk, run->off[run->len-1]);
    }
    ext[0] += r->fnt.space_adv * (!!_.len);
  }
}
static void
res_fnt_fit_run(struct res_txt_bnd *bnd, struct res_fnt_run *run, int space,
                int ext) {
  assert(run);
  assert(bnd);
  int width = 0, len = 0;
  assert(run->len <= RES_FNT_MAX_RUN);
  for loop(i, run->len) {
    assert(i < RES_FNT_MAX_RUN);
    if (ext + run->adv[i] > space){
      break;
    } else {
      len = run->off[i];
      width = run->adv[i];
    }
  }
  bnd->len += len;
  bnd->width += width;
}
static void
res_fnt_fit(struct res_txt_bnd *bnd, struct res *r, int space, struct str txt) {
  assert(r);
  assert(bnd);

  mset(bnd, 0, szof(*bnd));
  bnd->end = txt.end;
  if (!space) {
    return;
  }
  int ext = 0;
  for str_tok(it, _, txt, strv(" ")) {
    unsigned long long h = FNV1A64_HASH_INITIAL;
    int n = div_round_up(it.len, 16);
    struct str blk = it;
    for loop(i,n) {
      struct str seg = str_lhs(blk, 16);
      h = str__hash(seg, h);

      struct res_run_cache_fnd_res ret = res_run_cache_fnd(&r->run_cache, h);
      struct res_fnt_run *run = ret.run;
      if (ret.is_new) {
        res_fnt_fill_run(r, run, seg);
      }
      if (ext + run->adv[run->len-1] < space) {
        bnd->len += run->off[run->len-1];
        bnd->width += run->adv[run->len-1];
        ext += run->adv[run->len-1];
      } else {
        if (ext + run->adv[0] < space) {
          res_fnt_fit_run(bnd, run, space, ext);
        }
        goto done;
      }
      blk = str_cut_lhs(&blk, run->off[run->len-1]);
    }
    bnd->width += r->fnt.space_adv * (!!_.len);
  }
done:
  bnd->end = txt.str + bnd->len;
}
static void
res_glyph(struct res_glyph *ret, const struct res_fnt *fnt, int x, int y,
          int in_rune) {
  assert(r);
  assert(ret);
  assert(fnt);
  int rune = in_rune >= RES_GLYPH_SLOTS ? '?': in_rune;
  const fnt_packedchar *g = &fnt->glyphs[rune & 0xFF];

  int w = g->x1 - g->x0;
  int h = g->y1 - g->y0;

  ret->sx = g->x0;
  ret->sy = g->y0;
  ret->x0 = x + math_roundi(g->xoff);
  ret->y0 = y + math_roundi(g->yoff);
  ret->x1 = ret->x0 + w;
  ret->y1 = ret->y0 + h;
  ret->adv = math_roundi(g->xadvance);
}
static void
res_run_glyph(struct res_glyph *ret, const struct res_fnt_run *run,
              int i, int x, int y) {
  assert(ret);
  assert(run);

  int w = run->ext[i * 2 + 0];
  int h = run->ext[i * 2 + 1];

  ret->sx = run->coord[i * 2 + 0];
  ret->sy = run->coord[i * 2 + 1];
  ret->x0 = x + run->pad[i * 2 + 0];
  ret->y0 = y + run->pad[i * 2 + 1];
  ret->x1 = ret->x0 + w;
  ret->y1 = ret->y0 + h;
  ret->adv = run->adv[i];
}
static const struct res_fnt_run*
res_lay_nxt(struct res_fnt_run_it *it, struct res *r) {
  assert(r);
  assert(it);
  if (it->i == it->n) {
    it->at = str_split_cut(&it->rest, strv(" "));
    it->n = div_round_up(it->at.len, 16);
    it->h = FNV1A64_HASH_INITIAL;
    it->blk = it->at;
    it->i = 0;
  }
  if (!it->at.len) {
    return 0;
  }
  it->seg = str_lhs(it->blk, 16);
  it->h = str__hash(it->seg, it->h);

  struct res_run_cache_fnd_res ret = res_run_cache_fnd(&r->run_cache, it->h);
  struct res_fnt_run *run = ret.run;
  if (ret.is_new) {
    res_fnt_fill_run(r, run, it->seg);
  }
  it->blk = str_cut_lhs(&it->blk, run->off[run->len-1]);
  it->i++;
  return run;
}
static const struct res_fnt_run*
res_lay_begin(struct res_fnt_run_it *it, struct res *r, struct str txt) {
  assert(it);
  assert(r);

  mset(it, 0, szof(*it));
  it->rest = txt;
  return res_lay_nxt(it, r);
}
/* ---------------------------------------------------------------------------
 *                                System
 * ---------------------------------------------------------------------------
 */
static void
res_init(struct res *r, const struct res_args *args) {
  struct sys *s = args->sys;
  static const float fnt_pnt_siz[] = {8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 20.0f,
    22.0f, 24.0f, 26.0f, 28.0f};
  float pnt_siz = math_floor(s->fnt_pnt_size * s->dpi_scale);

  double best_d = 10000.0;
  r->fnt_pnt_size = 16.0f;
  for arr_loopv(i, fnt_pnt_siz) {
    double d = math_abs(pnt_siz - fnt_pnt_siz[i]);
    if (d < best_d) {
      r->fnt_pnt_size = fnt_pnt_siz[i];
      best_d = d;
    }
  }
  r->sys = s;
  struct arena_scope scp;
  confine arena_scope(s->mem.tmp, &scp, s) {
    int fnt_siz = 0;
    void *txt_ttf_mem = res_default_fnt(&fnt_siz, s, s->mem.tmp, s->mem.tmp);
    void *ico_ttf_mem = res_ico_fnt(&fnt_siz, s, s->mem.tmp, s->mem.tmp);
    {
      struct res__bake_cfg cfg = {0};
      cfg.txt_ttf_fnt = txt_ttf_mem;
      cfg.txt_ttf_pnt_siz = r->fnt_pnt_size;
      cfg.ico_ttf_fnt = ico_ttf_mem;
      cfg.ico_ttf_pnt_siz = r->fnt_pnt_size;
      res__bake_fnt(&r->fnt, &cfg, s, s->mem.tmp);
    }
    r->fnt.space_adv = math_roundi(r->fnt.glyphs[' '].xadvance);
    r->fnt.txt_height = math_ceili(r->fnt_pnt_size);
    r->fnt.ico_height = math_ceili(r->fnt_pnt_size);
  }
  res_run_cache_init(&r->run_cache, s, args);
}
static void
res_shutdown(struct res *r) {
  struct sys *s = r->sys;
  s->gfx.tex.del(s, r->fnt.texid);
}
static void
res_ico_ext(int *ret, const struct res *r, enum res_ico_id ico) {
  assert(ret);
  assert(r);

  const fnt_packedchar *g = &r->fnt.glyphs[ico];
  ret[0] = math_roundi(g->xadvance);
  ret[1] = r->fnt.ico_height;
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
static const struct res_api res__api = {
  .version = RES_VERSION,
  .init = res_init,
  .shutdown = res_shutdown,
  .run = {
    .begin = res_lay_begin,
    .nxt = res_lay_nxt,
    .glyph = res_run_glyph,
  },
  .fnt = {
    .ext = res_fnt_ext,
    .fit = res_fnt_fit,
    .glyph = res_glyph,
  },
  .ico = {
    .ext = res_ico_ext,
  }
};
static void
res_api(void *export, void *import) {
  unused(import);
  struct res_api *r = (struct res_api*)export;
  *r = res__api;
}

