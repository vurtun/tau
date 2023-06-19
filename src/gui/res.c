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
  "7])#######@lwfj'/###W),##2(V$#Q6>##.FxF>6pY(/Q5)=-3g6#5I[n42B^?#H&,>>#t####aNV=Bq#Zn%ssEn/,@uu#F0XGH`$vhLGOjX.4EXQ'$jg0F0&'+MGNse3=9>n&%J%/G"
  "F)u2(rM9(,FAuY%,5LsC/W0NUYVT`jius>db';9Cb.2j^IMxdWr1dD4AJr9.ev*2^V%e--1T#KDGqkGHe[]=#FvIh,=gOnEFV2<T'_`lLY8RtLcf%-Fo;>J#?e[qLH(i%M>@NDF%d3$f"
  "#Y@iLK,f;#dHU8GrBn4s(U:;#9M,$M+D3-G?n1YKwwH6#&T=mL>1dWM^T*$#<DW)#QFbN;GIsaEhpf.#o(ofL;2K;-6Xh]OmmR`EUOD-#8LM=-#M5s-Z:OGMVkm##YFbGM`T%+1@0*;Q"
  "(2###_D5G`fg(&P+x9B#6VQC#5hI:.+D>##;,ue<3P_-#KmFJ(2t((v@_R)$Bkb8N08G)NKF6##U.xfLceEB-T.r5/(,>>#AnRfLP:$##?6FGMP%1kL.k1HMaLH##IxRu-Z(ofLdUQ>#"
  "b_rvu?'vhL<s8gLb@-##E[.NM$(^fLR4pfLOYZ##?6El%eC35&:[jl&>tJM'B6,/(FNcf(JgCG)N)%)*RA[`*VY<A+Zrsx+_4TY,cL5;-ulAv$-4Ma#P'<$#sIkA#ig8,MiMG&#Xv+F%"
  "uCF&#vObA##JO&#EH7+NJLvu#:49.$K;G##Xj8.$LAP##Xj8.$6>Bp7Y-UB#)####+,>>##JO&#?Uu)NP-oo%:49.$&MOgL.JpV-gcC_&xCF&#etgA##JO&#PH7+NUZKM':49.$+l'hL"
  ".JpV-lrC_&#JO&#HUu)NY)df(:49.$/.LhL.JpV-p(D_&#JO&#LUu)N^M%)*:49.$3FqhL.JpV-t4D_&#JO&#PUu)Nbr<A+:49.$7_?iL.JpV-x@D_&#JO&#TUu)Nf@TY,:49.$g3=&#"
  "w&gQawOX&#3cOn*J&>uupk.1$%Ck?19^0%#W7OcM';YY#NCLV%s:^)4#3W&+uA3I):2Cv-oUFb3sJ))3#VYV-f(]cVM/Er?t93L3U17F#A?pV-TdSe$qH,`#RNTO0*X%##%,Y:vha-iL"
  "o*t$#dp+X7SYU;$WiB>#(8P>#1qP$B@,?;$-T1v>-Y(Z#3TL;$'G9^5b3:]$q+rB#U*TM'AkQA/=V&E#nGpkLNIng)&6Hj'TpVa4#Z'B$xEWt*?>A(WemPJ(tnwx.aoX[,H&.+4xd`n#"
  "V4OcMPCWe3h</Q(Y'ko/2)_e$4*9j34gY8%b+geZDq)##)&>uuwgv$$uSl##Pqn%#ToF6#Gl'e72$pq#Rlv$AXg2mfS4B##4mIYV'W/:9ne;?#a<@e7d;VY5Z`:Z#FmeP&b,f?nYs*h:"
  "PSIK:?Rap%>FIW$VZ:Z#97n8%]#M$#Bbe<$HMNT%.K;,#6P,##==LcD,Vu>#6,Y?#.i_v#=Px-#?L.w#-Q:v#EVQP&iD]_m@,GoYO9Q4F[%4'@dt(o035OI)lP`k'r.rB#PDRv$a&SF4"
  ";$QQ's%m'&1;at(SH()34mWI)?7%s$nbK+*M@i?#W%E_&^Ko8%x3vr-w[<9/$;Ls-/W8f3&,]]4RWgf1Ac``3%&AA4#/<9/fPpG3XS`$'MP,G4Rq@.*q%^F*/p^I*<07IMg^YA#X?hc)"
  "s35b@D1T;.HDav-a=@8%k,uB8Jkw`4GXI)4Mq>`-HH@e6(>m=7m*[2Mp*0_OxxMT/QrX9VfKXk5<4j<>sDQh#'tLA1^*OM0gkl(N8B^Ki7s3&%f;U>$2sA6(>*;+*AqS2'S=53'>q>B$"
  "5<iC$b5$3'75-T%8#FA%X`vp/$G'T7[$o8%GmO?--AwW%]&`2'@_,a*Ok<t$0du>#BHm5&1xSE#fuVS&nQf/1o05'$OZ5^=O&>?,l)'@#kl;K(l&c=$1L1#,KB.+NNx&X%N<`>$e*vA#"
  "^JJ-dN,NV,C30$-A]._#<tZ>5+4nseF&D?#w0)`+_LR<$WrD)36>$d+KcgHQBDW@$186X$i[Oq&`h`?#J._b$P?.Z$W5U<.6;_G+Rv&T&@6O<7aRpf)kSmn&Q3-lf<LYj'+H0[#88_r."
  "e>da3PemY#@ptm'6g3x%xB1-M7P*s%fVG>#5%RP/Gmx5'rmKD3&#,44^E;O+_+ZLM&g;k'X]h@-a4e=.W7QP/b7AT.-5YY#8:$DE*(NA=AdZ`*dGr80(Auu#t#xu#ZsJI$%),##Y:j'%"
  "pFU/#c4fL(shFgL6D/x$p>(l0kH7x,Vs'Y$0qwX-Unn8%_%NT/+M>c49XH8'uIMT%`pM,*36RG*iK%],5<,-2onmC#v$S;m`(t**Iq)l]6FL#$Kdl8.(PUV$2n)q/NWw.:+M-TB[?.%$"
  "U0,p%4u._#mBrD+2o_;$fhK&#mj[8&+WSW%GO_,DCxMd%pOns$<Nc##7+%<$<T1Z#,?;,#9DNPA,Y1Z#s4)N9n1ZG4-fj9/A:_f)V$F9%no<i(wD1l0XsHZ-eaFx#eE'P(@cs'4m>:Z-"
  "jBFA#Lov[-u?lD#I4L#$EN/R.W*w5/E+p?-NmlYub(96&9WD:MGE=30,Gr2'<_ET%<q@K*$BI1gfX#JC/AU1MjH.QOagt5&qxH['q3n0#asoXu7U^PAU4LS./j>G2otqf)8V(Z#qE,+#"
  "WoRfL`TMcD-VL;$Z/uoLgNN;$5N(Z#OgVF*+(-(+wc_l0W:AC#mP1I$I@Gm'():I$+r$x,^s8I$$#H=7nM9I$0s9M)-><j1AS))3ql@JLWf#@&3_ZZ,9Q4U&3(lU&Yo>[&3[ZZ,CT8U)"
  "iFvGidYWSd$gvS`2;u/l@_.u-3-7ENW+TgLfwe6/`>uOHvDjDMOF$##&&>uua9LhLD:d&#Dc/*#sE,i*>EL,))Duu#+oBj%dL,+#6(7s$U0[0#+Mc##V<[0#6lq;$I%Ub$*AP>#'8>##"
  "DA<T%4E;,#Co/QA:X<T%)GZ,4q&>:.Npp;.(v$m0Sg'u$I=^v-Ut;8.X5Gd3vf%&4Z;5-35NCD3:sIL((sCm/f@0+*oGSF43[D>&3UL,3tD1C&WM_T%teS#cu1rC&U-Cu.VmQd3*S2B#"
  "@ZO4:IHHc2g*HZ8B5csArk7Y1rko>csATAcZr*GM+,@]-'ArU2[TdlA)P.R&5jou,0);D3HI<?#'2G>#uAP##-V1Z#dmRfL'o-##<ZU;$,9;,#7JaPA.fU;$ut_TrAJ[v$a7&_od]tfM"
  "5RqkL,m(o-ucCtUvUcj%o<uj.aS)<%mOv@M=+I.q-m%)NfITY,e$W)+l=ofLSo)($Am@^#Er'kbPS,%,UX@s$L9@K)[(Ls-3GO,MPKZd3d^s/M_Da.3SiY1M5KihLCIL+*UXrH;j)%bW"
  "pE#W-`T5F%AlEx'iwpaW^hv4*H@Fk'e<qWQT,;h13#nERW>uu#dk53#@kMV7MV,##I3SnLaGYcMEB@C#S$###.KkI)qPCl0mw4gLT'MB#T4gA#W_l#$Mt=V-EfS]uo:/b-O2+LPtuA+*"
  "EsOm'#UHIMap?'X&l4hLrHf-#,&>uu3Y.d#@7>##MXI%#^3=&#<Ja)#X%T*#vnC$1q(Jv57u(Z#ZAOe)>L%@#AO[s$Wu<5&D<K6&i#Ml'MtxM':UwW$RIr@#>.%[#bd'7)*J(v#FU8r%"
  "A%[S%(>Y>#ldOU%pAWPA+S1v#;$_Z7;Y[[->OZc*iZbX$l4^l8<mrT9,hTZ7#x.E5OABj2'Q:?-jJu',FBo],6=Qc*IK.iLmEL+*SU@<$&=LhLihdJ(1E1l0skY)4XEW@,j^Aj0a+O_$"
  "HPjc)e#NhLJ6,H2v(V:%Ynn8%a%NT//,-J*7Lm<-GdF?-f1jq/7Sn%OliiO're$^4QE15KQ@Mcu+k[**,pxZP^,.)*5fMh'7B=m'mi*n/_4Dgufi7q9R211M.6M*:'h02:vA[(:xZ%RN"
  "5vWuLj.3ulxFClL@fdi9FeY&#xnL?@S6O6)aX18.0-$5A]Hx7Ip2>SIC]M@#U$?4EIQ4;H(Z+C$mwu@/_.jMKoqA6&:A*9%(QTS%>O<T%TYB>#*DP>#CD>?#.l6s$:Dx-#>nSQ&Gf4.#"
  "=@<T%Ut%x&)2###^qi*%T8<PA,<(<-SSO`+ejZ&J0PQc4+cjT/Jm';.PC'(42d8^+o9OA#jD<r&me_F*CCS:/7c,0)J]d8/]$vM(2.^C4abL+*irv;%5C;=-7'^,2]UUa4%[*G4FxJC4"
  "K^Rl1bu?#6R@=K2KB0u$xS?A4'U1,)XJ=c4uppo/*W8f3fd<c4?0QD-5I(T020e,*nwk%$oZxk9tU#B=t5Ro'LQl8%3k,inX(-Yu6-+#)).TD=Fx1?#gNIa+n0d#Be*,9%<c:Z#?$LX%"
  "fBwx,sKtT%N^Yn&FL[W$-l?8%Q32X,pZk/i7Y(v#Wu[fLU%bN;1GNp%C@Mw,M4D<']8IV%a]Q<(A,3iKt7WT%xtj6/hVI8[/UOm#)?Rqnis*,;bc&^+ClcY#nG)4'74a8%PYjE*-.+T%"
  ">&,=7:mA_>jW^m&WN-)*>dacu..%1*`O'_*pee[#@JlY#7rlYu8]4X$i`jM2_=,gLjniY&Ma*W6+WH(#3q5Q)ThiM0]0s20EA.?$*ORG;57@W$X3;#n`tZY#0oLZ#@MUS%;1[s$OX)%&"
  "v#*gLPctO%lJ5b+tTcn014T:/o`'l%m=+gj^N[g)s46J*RWc8/+gB.*g;M#RJ[#,MD,/u9`fSc18/E&7cv?_-cg5WEI3N*:Rb[v?b####%HpS%UU?##5f1$#UKb&#/pm(#mw1G#tn]T."
  "1uU?#rDnf(lW0[[kkj]7$K<X(48)m&B8^Y#MHsxFF%Is$<=e8%$P4]#Ihn8%hFro%>o:$#`s^n))J:;$=7:,DH72v#:x6W$>Mx-#]G3.)xoML+GB,/(js3eDTt/U%jJcoLlG1U%D1Yj'"
  "6[j.;%fa9DC9j:DA5FZgO++TB0)4^#tL,)h(3,^?M/ON;&lGq&C:.<$xKN`<^U-P;M&sT9e)8p0H/,G4GdE.3*fj9/Kt2)+adGn&U9tT%K-B6&;(%<$%oP29f0LB#R1UF*uMk+N0T'r."
  "cPwV%OHF:.6L/@#$DXI)*kRP/QV>c4)`OF3K<:)3+/rv-I_[@#R:8>:uOkA#FRS[+2mQ6:t[u%F21aUK/`Am&0$n6't6XP0_1s^#kBnv9C;]6:jGg;-w9*S,?B)*ux-nlbW>^w6HsVR'"
  "WrM':iqYIqC?ihL?N4D<F@NeFU$of$/ws+;4;$p%-oI@#^ldC#)IIG;xa-^4r%%w#>R<9%fMwo%?UEp%UA[S%PWS#G'C,H2(Duu#D41,D0oqr$KUL,Dl/Uh$-OdZ$.E&##fZT=8BLJ88"
  "w]]v$g+,']gh4W8lX'_SPT^1QM7t]7nAu'6<]A`=lBEm;@Xk'6`^[12.HnY%&S)'64m.)*edB:%HXX/2a)DK1+Sx*3Ab#V/b.i?#E'.L,4cK+*XIDX-(lNX(^K&o80F5K1MW^Q&61Rs$"
  "16)6&1=Iw@<$GR&0CRZ,+gr#5sb&m&[t/R&XXZZ,(L@A,bN&3M,)nC#Pw:ZuZkc>#BlZT%Smap@_2_Z'pQ+Z&du`S&lD0t&PaG^=#9Qo3cU:%,m>vC-`E&%#v7?68u7CW-Dc/*#'LTP)"
  "j%O87q4[s$.>)6&51R8%_:S/L2rLv#Kn@jMZn;Z#+nS^ZSuHW.3Uov7'I-H3nxQ^Zhk)/:I5Z;%mI2W-tI)QLhl4c4U7*u7&QLjC]PcB,QeuS.$4fQ&*tFg.XAj8%wSq#.2_jB,B.iB#"
  "IgTe$^r:ZuIql>#/NUXRtTen(/qs+;%im%=r;L@#CQ2614o:Z#-cBW/Vh6Q8;$g*%JORW-I]MWfCe$68fYL/)2M6<-8Xpk$XjVa4ktr?#cJ?C#=V&E#;lm=72q_F*7P[]4?R$v,Jn@8%"
  "]_QP/Oaxu6Wj<X$Z:lfC@s-s$Xb*X$_)MZ#D5&NKXu]#,aR0)Nxs]w#ft1?#0cjT%HV#<-==^A.j^$d)p?$aNSO`].AI>#Y)>58..qhV$Aeq^#+9`G#_?Hlf%2TZ$7lcYuSS,<-qr?.%"
  "h,dxb_Z=g$YxGg3Ul$W$-/5##sH,+#/VCv#l4IqL&rQ>#:Ax-#%,>>#'3G>#cEJb%gWQ=/W`:5/)WS#,/D*(=fU(*4;=F]-FfffLFG_Z-;qD)NV@)aO#'pP)/GD-(])iO']xW**BnvW-"
  "$p6C/&f'B#b>wgLX3`HM_sTM'ODB+*:Af*7teEW8J4Q:$K-q)NP^b)##XMu29KWV&6(D?#97[s$Ba(v#&gM*e:0C0cO&n*%B#o>$R*Zh/-p1d>E_s-4UpxG3/NafMokK,Ns1E.3QX6th"
  "fYu`4+rbA#1HlG*lq-(QUc6=.Rq@.*Rfl#%Yh?lLFxMv7.r]G32FLB#*S-7]?E&;Qqvu7eAC=4o=bU^#ndo@u?;m]P2LJGOvQopE2B.u/0fU=-Ie%s$9T5W-qT[w9F.xVO..`S.UXb>-"
  "7G#W-Hk;c.+%6aNpn$##$&>uu5bnw#QKb&#^u)'#_LkcM8[UB#^T(v#(H(v>1bd$$0WS>#vK3D<m..<.O&ee%1%=g;lC(a4#::X%5ue20`ZD.3oe-=-%rA[K^Hl`M/g?s,1Bn$'TA*XL"
  "_Nu`M#lVAN:-S=-KwP&#Y>0,._A]nL</n##O?8W7F2G>#JE4OM:gP]5)<]Y#1m@-#wZj%=5Oow#+tZ0)7Ij?#s0Y.<9%9JFWh?lLJ.JI*`q)i0_twl/4N?>>;1OB*8[/YGH,cB,2CDk)"
  "u^j[#q8._#_Ifv$JPjq,;(4'*JYdg)9O$W,4a?['wL;%#<(L$v;oB+$*'D$#L?O&#9or$-nP#R'*DcY#uJ1v#>'O,5+UfiD*S:v#)9A>#3v@-#&k/&=/]4.)[J8^+]U5l1Tu/+*x6LG)"
  "&Y@C#GO=j10[$Z,WvB:%+ix+MFM-vLoq/E#gfTM)ic]m<aHNa#T9MoW[3eD#>BY@#L49j##S5-%gwfN,hSDF9T-)8W_HNA$ZDVs;-<-x)(2###?G.g(]WHa3aH`,#]7$5;m:es$Bc+M("
  "4Su>#YQlf%W178%NkU,D6Vl##BuY;':rq;$(2>>#Bugb$2+wo%`5:5Mjpxo%7d:?#-GUD?h[$l;4$UZ7OT07/TKi=.,v$m0_=]:/x)Qv$t@0+*s<RF436Ck0OU<9/%Fo8%Wg'u$:?KW-"
  "pm<.btnZx6llE.31uSfLmU^Y,b]DW%+A^;-gv><-A@D]$2T)<%N;.WtN9wo[p-'PoigqF=U,>)4^4]?Z@e`-*vdAt%`d,6&Gg1q++?1Q/HpQ^$(3#^=;SoAusJ><8@F-IM8OYm)s:TJ)"
  "5B###+,Y:vMjQ/9<M2N9hxQ0#r7o%JIig6:&cap%xTA<0owr;$+v8/=aDxL(@c1v#$F-u%2JY##.,pq'<Fi?#A[NT%H)`V$7YCv#IUM[&v$dm8B4Z;%7`'gLf0.##4<,##Fk;:KJl)2F"
  "aOB$Av-Rl;(LEH*8$o0#sKXt$>XSr8toha4OM>c4qJhR/I8dG*8ev;%hfk/M/vw;)V6nDNhCld&W8tfM4ud299q]G3n7*EcYh?lLGOJD&TRWm&KvpG2;]ZT%D6t5&8(;Z#xkY#uf//_P"
  ">?cHO]=Z<-bsVqL<L5WOH+-ZE=1,HE>x(Zu>n=Z-8Vet(s5+9&'R[(<.sHSfofq58)5###:J`onk%ZS%U+PV-2YNY5x^>PA@HGB6wwSQ&8RfS%]N][#&IRG;8I35&P$r,D0oqr$Gh-dD"
  "41RS%(FMj9@ZD^4=W_V$(?M,#,;G##>Gx-#1>s1B$),##C<qZ7KbTu.#`K+*pueh(*v$m0M$>g1B?uD#3s(u$S_;p7a(wY6kX'`$=FLo*N2C<-I7`*%&D-H3d$Whh#iucMmf)g(8a_7@"
  "GqWT%4ZYm&b'Ls)GK]+.N1.X7OxvD6Rd&_<:JO]uQhPX&4]fU)pCw5&gs8I$GL)r7Uic>Y)5###%`[on=$ru,X>Tc;Clg%Fe#VD*Nd#a?I-rk'oPaM'`C<2'jNA8%AGIR040Sk9IiqV$"
  ";@Jw#9]u>#,Pl>#$b3H;O3EE*=MuY#wCY)'p/HpBUPrk'to<5&Y<06&GWCv#RKfYG1uL?#E.1/#gPeH)M.A[#R-92'X?7-D2%78%Xq6dDFHGJ(bd[-D2uqr$JehGD/r$W$7,Y?#5xhZ#"
  "CT/'d8>7#8R]@a+rYKjLT6kZSi7jZ#01bYE[%$REHWfREt/MK((c'keWOBmCNIMT&[>aP&aDb_%Gpq`?,A51<X[a&J3dtnD>j$x@1JnA@gqdL<=?6<83poY6/NKa+uFTb*=*wG*5TsH*"
  "g=Us-vqXF3J29f3rnm=7rFg+424mg)o`0i)]/2,)bQv)4%8o]4QIVs-;ntD#_R(f)NCI8%0*0g:gZP2r$+p+M?d8f3dx[v$g]/J3tsaS8EaLd2J_l?6MK4>-?NeS'W8d7&di`s%k=[(,"
  "kcjP$JB<-*_4B'+T=XB4uE,;/ktdi9RoMT9R),O6AjmD*+3;&$XbwR8`NHa3e7]C#M4g1($i>v,h4J5'/;P>#_Bxt7AR158cM>?,J=6c*Ob%@#KQ4>-o:]i17bIt-6]J(%r9pm&0Es(5"
  "]v_:%^4.n9;3YdF@K.`$8:$DEe-SA=btHP/LK[i9/0?PAZbuR:>%`Z#R$gS%3l%_#k6M)+.fh;$u;G>#5`u>#`FQ?#8[sp%AYx-#;=*9%FSx-#1Pu>#BMx-##5pZ$/p-W$4jf>#+m+*="
  "e@14;>&V7/5jGK)5vvn&rBbufV^;E4vfNt?quPg)Z]B.*+;hq7/sbgjA,B.*e&kuJG-YD#^;QiTD/)N'BdKT%X@[T%#gOgLBQfc;JE.5`A<kR/%0,6&F(>JCDNj8&`;U_&Cg4R*aD6C&"
  "W#0k1=BUj$O4bd)f(EX(N%<5&BEon$*rkA#Bu><-:+,-&;4Y&#Q3n0#cL7@#v*V$#2iMI;$hl)4w&<a*%&-*(/f-=-Se&N*#YG,*T`l*%>uXk;(TeU0',Y:vahv$$d@=gL$qp$-R]->>"
  "<*NJ)8k+j'3RSM'IL7<$V4^f)/`L;$r/5##-SL;$[2n8%.i$s$fGMfL[P5b#8G4.#5uh;$-a_;$7pXV$7SlR(^_a9/15S1(0iB/)Vkes$'aMG)77Yd3$8H>#qc4c4,0KZ-dN1K(P1E]-"
  "oKVN(A45x>#E(a4%`B%%O_$X&n[s`#SY9N]G4FA#ts=F%@O3t$b3Xm&_U6i3B$Sb._Y@u.RRTd3LATw6UF8>8[Ol[tArOj?BKCF&]5L5M.FNZ#Crhg(*ASd3+]M^#+n/gLhSF&#^QwrQ"
  "P]2/(-W^f18i&://s%l'G.2?#3M,##wmCC$UY]#$6CC_&NYafL%8#Z0]/1Z-[R,jBF4(<-P_j=.N]d8/f@;?#(*^s$Af)T/YLd5/ZI:E4o0`.32x_1&=MbI)]]iO'pi]*(%#ko7&6_s-"
  "#>f8%0PPYu&]fE%])$?$hW@VQVJFbNS_v_%Yl#?#n,`*M(G+'iW0pi'$Q3%tw_wZ$CIb888F@[0AB-N'1/,r'nL-`%#x7vlH;`$#(AP##acK^4lq1$#8(V$#EDW)#_>0T7GqI&69AP>#"
  "cg#a3'8YY#ahE;$lW0[[lkj]7^i_v#5%%<$RH:;$]ub^G*P:;$<=:,D3:iT.5o$s$#p?*(^r.[[_xH]-VD2:2DMe(W?lmn]tpJj2rE)?$E'^TVLfvZ?,%o/*UqWW8QKrk1V?j`+R=rr$"
  "FT#r%;fDt.H,Jd)=l65/OgZ)*=.Y.<a0>)4IWFs-1rSfLUx;9/s$+,%6ZKZJP])q/Tke@#._A8%aMhs$,h$n7D.#D#ebJ#co/k-$%p$?A<qP<.T:R8%^B53''NLD&9TDW%i94kukX]]F"
  ";$W4(p:28(d6jx7bJ^rQd^Z]u[*4^[]4BH#N-tt$NoCv#)RJW-4@V'87g4?#12m@O'jLe3mNEk'`^_'#'&>uuDn[)$.#M$#m,_'#ilS`7049w#^-&N0G/,##W=iV$,ZWX&N#_B#D4dY#"
  "(QO?#ge'dM1x68%]B,##(>G##p4SgLTn2Z#IR)%&AqU#$]wi*%lQ3v#4HuY#(p;O1*gMk':H[+4NoC[6d+821FOL+*'7v?,@bJM'Dw3t$Bp()3eRYI)-><j1I=7`,(.O]'dk]['>U?1,"
  "ln@8%<Lae3gI:VZCrLaG`'bVF*`w%N^2b>H;I]cMTHp.c)6212g'2</+`.TA+Tq=>OnlS/PnlS/TG[ih=XC^#6icjL0@#,MOXjm9Nqp*%``N&#G0b^c[v6gAeI1=#K<5)M9%IxktT9EN"
  "O7qXul'J,2`^^w'OMYY#0]Wh#N&###82'DEV9F,28+KDt.IKfLXdwbN@A=/M1AAGV>Lcxu<OY&#5d1R3#Y'#vc-:hL;fjfLgRDiLu46EOX#S/Mi0fu-H*FcMC?6)MLTRFN><oY-Ant9)"
  "oJA_J8j(LPBIs'#-:-_#N;7w#B6uU$2J4M$VnR-%@7dV%cUSA&ZCGf&7O7-'cX;w&OWMW'#d)H(2[>R(TA/*)A)24)5n^k.7-j<#$0D=/[id(#GeMuL<>(f':lA,MYKs$#U:KV-a.v<-"
  "%lL5/$),##<uC.NaF6##CO7/N).gfL%0#0N*4pfLw`l0NWXQ##_/_1N,@,gL@(o2N]w)$#<Ma3Nc63$#t>T2#+MC;$lZL,EZLn_&Xb+/(p1i_&WRJM'*ci_&5c4R*W7j_&*NC_&hhj_&"
  "cN$)*XBk_&8l4R*80l_&RKGY>)al_&:G-5/KXXdF>M_oD2<XG--]V?-@ZWI3?G0H-b&K[5t5kjE9$OGH64o+H`@viC0jXVCDSRr1@vaONblvRC#*N=B^+VENf:ESC7;ZhFgLqk1k<bn/"
  "N)xUC&BXVC@#?T1Qfw33=b.F-.JZ1Fh2).?U/>fGqEDtBM<,FHr(622-Eh14+/G]FcgXoIT1PcDH]4GDcM8;-8kTq)KD-F%Vm>G232;D31x[PB]kX>-$h#(/wC-AFFoK`E@OS5BLPQq2"
  "I]+4+Hm_w'_fl-$93(@'I>Wq)L:He-=3G_&,.UiB9*;)FA<Au-^s6qLW4urLLfipL2<dxP+#ZoL<@2/#/H<RMpQG&#Z&BkL@+5?-2X4?-`wX?-U4%I-C[l0.:/'sLfJhkLO-lrLp3oiL"
  "=3:SMPLG&#v-pJ._rt.#LK6,Eq;$mB%v=j1EY%44G-QY5+v4L#8;Skk2,BP8:``8&l-WS%=JdY#,VC;$0o$s$41[S%8I<5&<bsl&@$TM'D<5/(HTlf(LmLG)P/.)*TGe`*X`EA+]x&#,"
  "a:^Y,eR>;-ikur-T3;hFwf1eG=$Bg1@S4VChougF_MBnD,-f,M(DnM1?CkGHhquhFqc2#G(jqc$3r*cHk(Ed$c?DtBvk`lEoM3^-M*L[0,m7qC41`p.nZ76D#k:C&+#fuB&gkVCE[f5B"
  "+dYhFI/'vHmxj`FIFknBa-+F$/f*cH47MU161=GH=7qw/N8?;Hf:AvGt:,</qI/>B1n7,MChu,M3u(RB,3fUC4wU_&Sb;2FnAFVCv<]iFCUf6BQ`0[Hq3%OXqhK=%*&VEHuW+F$#BoUC"
  "Zf3nDfZV<Bh[G<B*Li+MJF-##$)>>#K*I^OVAj`[vKRV[lQG;9^A@/;;V>'=#K2^=f*Es3qMaZ?\?OJDO.gIm#lB?h%9fSiB''*h+lWg5#q@HP8=#%6#sqdsAD0-Q#+^+pAj9Hm#b;_;."
  "%Bd2$23eZ]O38%#mkm>)G,nvA-Q[G)*g`,/r%Im#Sm&H-0`3jLON+9B7$$a<R]rY#hB#[.w<a-,X0F7/O,S@#$'oiLcfg88Rf1,)cTQ<.'$VC?U*'<*PxiQ013P/1^j`W[WL35M'&i;*"
  "P8PI-Vj=2CZ<)u<XDdv?bo92'TeU<9xHGB?sB3p.-^hD?.+ba+,(TV[XtYt(RG.g+Z*j1M7]B]?)e=vM/9bJ-g*`*N#.RV[XulW-[?)@0fg]C?TNJDO5CTV[67&vAwdY>-A<W8&[>YD4"
  "qhnF4X3Ya?Lb&V[]%X)'%vi5#)Ti5#>0u?-NCi9.X;Cm,f3Y^,C0fB(pM?X(t':k$84r`?HE[][W8p`?&=$5..#/qL0+h88`TDj$*FTB-_(o5BYL6=%kaQ][@S2Z[Ve[d+p[j6308vW["
  ".j+huDQg?-M1)NB%Cr0,&T-a+EQZe$]F^lA,MFj$c?i0,5:IDOgG'REB4g[$#RW5/(Egj<K]1`?o&&a-8C4g7m2Ok+3uZV[J#2]04El/1KYmp0o'to77>vV%g>A>.Y1RV[?3x8BIkwpC"
  ">a^^[<U'K%4#],&Kc0eZSbapBBR8MBOv^E[?MZa?Jb/pA:jr217W101B?M$#nSJT0r/RY%iMc98O#8p&gvvA#an5R*3U'rA6rqF.)WoE-G/WC?f+^]+3/Y-?Wj@iLpDJ+/uI,<.[5K[B"
  "%Zf,7FSBjLJ:jfN4..]-^tG$gkP;n&CeimLQN(;.>X&[-<v_01tQ5293Y)w8=&i01]2[?-FE&R8oaT`<*#DT.=<Z][9uY<-%sY<-:.;t-?6@hLT_vNB2M/?-C[SiB87BQ/3QesAN)NE-"
  "J+g886uP#)Gs'D?,IAw8_,tD'QN1W1_dW0;l6Ne=Nr.a+SX`=-d[Y=6k)q5#&a0D?<&<R8#rD?8j+wZ?sF:Z[>cMv[/>^;-3`uS.d>N:.hUl`1)d7Q#xiD^[oV(HOsrW*IL&$[-1cmIN"
  "D/=fMqoQ=.]jL^[<chY?,s.m9%gIm9x:$p7t4b>-A1,])tiq5#HAGp8tI:Z[ko?Q#ttdsAX:Mv)VOo5#m`'D?K.g88GC:a'F1uD'WF^lAH?Ws^Tr,##G@%%#uY)HOi`1Z)E?7C#RQdU%"
  "aXWx0<:mS1P(:-mcRtN1t`/t['lu8.q;Qm#?D`)+2C1,M>ci11d15F%?k/t[+lu8.#ai/%CiwA,6[U,MB%821h=5F%Cw/t[0u:T.iGu>-Dk1e$ik>mA>rTA.#3-Q#3h'Z-_eL21UVH,*"
  "Y8JAGh&fM1f<Q8/ZAf]G`$1?-e&S&(GHnP.A,)<^T(cEn%^%Z-qS`>-sf@v-fr<F%t)#kkG@pV.fr<F%v/#kkIRP8/fr<F%x5#kkKe1p/fr<F%$<#kkMwhP0fr<F%%/r?-rLglA]6-41"
  "mDKs*`U>?.q05T8]xn5B=]fX[uUoM16MkW[^uf>2]7Gv2,s(m9i.r>*]&6-5FqkL>_gCs.;7mS1*A%altkBa'be3Z[rFFv2Zbr>#[daP2<0+jL-Y2u7[c:H.r@r?-b0#O-X0^kMj6(t8"
  "-]dV[c6;WJJw[:.CN56#hwkp8ovGu[FF$.M?dh41n]-na%?ZqA#'*9Bqn?p4&XV=6.(d1Vxg,Q#%UTt1oM.51BG$)*bf-@->u68.==*p.t2Gx9h:_5Boq+h+O`.Q#tmn55NYWI)f3<m5"
  "lS*)*>(#C/wF)W[L]5C/f-A$#e-WQ8m5Hu1ndR>6No,D-33YZ-t4m$0t5`/%=[fX1=eE)BG'TV[l)NY]mnc;-rZW5/35R>8Y1RV[q9IDO)D(XLhB+tAa)OW8f:JpAvmoi04bpG2/e09C"
  "?_vP8<QYC?'3*9BE4`T.;se2$+)`@-uc7'0wR5<.t9Xc+JmVE-Vb0)Ms.XvLA&3qLOlji0hq_H-A&lC.`+o5B88BM>'ah5BsH<p.N:G#$9iYQ:A.YC.AFsM6FiQp.4(RV[E,JW&X/o>6"
  "wN8j6D)nV[k^NmL#nY?.%d+c-EMOk+Zg#<8huQ/8lcMW[S)+p1o+;Z-JEGO-AqYhLKr<c%wTNp.`mdM:t<wp7S_XvI.]FB.9(7W-9d2nWAOH4OZ%x6W$UC^[DfO*QA(CP%-+o<.:BtmL"
  "dMWZ[&*si<BPUD?]<3p1pqMG)nsO,8A/gD=8i5M,/>5M,l='S<KE6T8fpvo0LIYq8gv)p0*fR>6?`f88T:c^0Be)Q8`FZkLZ%KNMdN7p.?cfs7'%f5BErf886uHs-OsVmLgrWA-3Fa.N"
  ";.O61^T6p.,T*)*TUU@-5vfa1=)pDcAtGm#s-kpL1?T>.c^b>-hNUX%E]72Nd$X61:^po0hY#<-t7V9.#T_;.^IC)N<>gS%nAlW[sRV/s^2$6#NYWI)Kh$T4voD#;DW30;WCD>#2u>p4"
  "8gwJ;g'WlLoA^nL'?B49`f-@-#[a5Bt2)H2X)xG&1uAv;&GxV<W=)##jS,<-tS,<-(r(9.T9?Q#+TSD=CHNX(3p3O+dh%nL=mGoL%+p+M#sPoLH:)=-32#HM31?81,%###-R+?-'Z5<-"
  "*%29.^?HQ#6M-v?Gd8:)>M0L,42KP8IFeV@%`bA#.J)s@[J9:)-Lf'&;%aSA1,S&#1_=?-.T,<-%V,<-K,JRM9lrpL6:)=-Du@Y-gI(F.2UJb%%0Pb%EP4O+?_tiCCHNX(rx:_S.GCb."
  "s%;_S9bpfDwTq'&u+;_S;tPGEwTq'&w1;_S=02)FwTq'&#8;_SCZ7aF=Y4;6Me.<-F,vW-JInBADKGC&gR*C&AimW[8),C/e^SmLJ,>;1=m,p0`iF>HY@/F.a3<F.fk/F.#c'I6_gYL>"
  "Ls:@'rb_VIjO/s71X%sIeNZt12'Sw9X9BO=NjGC&dRvoJV%3I-o2Yw0Y3bR<S2;@'AQtlKeNZt1S#HC&c)V*@+i+L5VwER<U)HC&k<4/MV%3I-vGYw0Z6bR<ZG;@'H;2,NjO/s7<#MGN"
  "eNZt12(,L5W$FR<]>HC&r&HDOV%3I-Y[n=19buu#maQ][Pv:N$q=GB?NTFY-CDgp0G,q^[iZi[IqcMg1Ti]6_A5pC?f2DYP&Lbc;_qw&GtNg58TKDM0/UV8&?GkM_.1NP&Gw((&en%v#"
  "oS_v[x7@4=J3:gN1Gi9.P2])0;J=2_l+<=(;8Y^?Er#0MwIFh%SNC;$NbE->l4Qn*u@q0#ndPM']XMuu@LJ>Pq<^l8FqT'%[cn^$XZY%f]$###";

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
  struct mem_scp scp;
  mem_scp_begin(&scp, tmp);
  unsigned char *img = arena_alloc(tmp, s, w * h);
  {
    fnt_pack_context pc;
    fnt_PackBegin(&pc, img, w, h, 0, 1, 0);
    int ok0 = fnt_PackFontRange(&pc, cfg->txt_ttf_fnt, 0, cfg->txt_ttf_pnt_siz, 0, 0xff, fnt->glyphs);
    int ok1 = fnt_PackFontRange(&pc, cfg->ico_ttf_fnt, 0, cfg->ico_ttf_pnt_siz, 256, 32, fnt->glyphs);
    fnt_PackEnd(&pc);

    if (!ok0 || !ok1) {
      w *= 2, h *= 2;
      mem_scp_end(&scp, tmp, s);
      goto retry;
    }
  }
  fnt->texid = s->gfx.tex.load(s, GFX_PIX_FMT_R8, img, w, h);
  mem_scp_end(&scp, tmp, s);
  return 0;
}

/* ---------------------------------------------------------------------------
 *                                Cache
 * ---------------------------------------------------------------------------
 */
static int*
res__run_cache_slot(struct res_run_cache *c, hkey h) {
  int hidx = hkey32(h);
  int slot = (hidx & c->hmsk);
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
res__run_cache_tbl_fnd(struct res_run_cache *c, hkey hash) {
  struct res_run_cache_tbl_fnd_res ret = {0};
  ret.slot = res__run_cache_slot(c, hash);
  ret.idx = *ret.slot;
  while (ret.idx) {
    struct res_fnt_run *it = res__run_cache_get(c, ret.idx);
    if (hkey_eq(it->hash, hash)) {
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
res_run_cache_fnd(struct res_run_cache *c, hkey h) {
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
  for_cnt(i, args->run_cnt) {
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
  for_utf(&rune, it, rest, txt) {
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
  for_str_tok(it, _, txt, strv(" ")) {
    hkey h = hkey_init;
    int n = div_round_up(it.len, 16);
    struct str blk = it;
    for_cnt(i,n) {
      struct str seg = str_lhs(blk, 16);
      h = cpu_hash(seg.str, seg.len, h);

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
  for_cnt(i, run->len) {
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
  for_str_tok(it, _, txt, strv(" ")) {
    hkey h = hkey_init;
    int n = div_round_up(it.len, 16);
    struct str blk = it;
    for_cnt(i,n) {
      struct str seg = str_lhs(blk, 16);
      h = cpu_hash(seg.str, seg.len, h);

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
res_glyph(struct res_glyph *ret, const struct res *r,
          const struct res_fnt *fnt, int x, int y, int in_rune) {
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
    it->blk = it->at;
    it->h = hkey_init;
    it->i = 0;
  }
  if (!it->at.len) {
    return 0;
  }
  it->seg = str_lhs(it->blk, 16);
  it->h = cpu_hash(it->seg.str, it->seg.len, it->h);

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
  static const float fnt_pnt_siz[] = {8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 20.0f, 22.0f, 24.0f, 26.0f, 28.0f};
  float pnt_siz = math_floori(s->fnt_pnt_size * s->dpi_scale);

  double best_d = 10000.0;
  r->fnt_pnt_size = 16.0f;
  fori_arrv(i, fnt_pnt_siz) {
    double d = math_abs(pnt_siz - fnt_pnt_siz[i]);
    if (d < best_d) {
      r->fnt_pnt_size = fnt_pnt_siz[i];
      best_d = d;
    }
  }
  r->sys = s;
  scp_mem(s->mem.tmp, s) {
    int fnt_siz = 0;
    void *txt_ttf_mem = res_default_fnt(&fnt_siz, s, s->mem.arena, s->mem.tmp);
    void *ico_ttf_mem = res_ico_fnt(&fnt_siz, s, s->mem.arena, s->mem.tmp);
    {
      struct res__bake_cfg cfg = {0};
      cfg.txt_ttf_fnt = txt_ttf_mem;
      cfg.txt_ttf_pnt_siz = r->fnt_pnt_size;
      cfg.ico_ttf_fnt = ico_ttf_mem;
      cfg.ico_ttf_pnt_siz = r->fnt_pnt_size;
      res__bake_fnt(&r->fnt, &cfg, s, s->mem.tmp);
    }
    r->fnt.space_adv = math_roundi(r->fnt.glyphs[' '].xadvance);
    r->fnt.txt_height = r->fnt_pnt_size;
    r->fnt.ico_height = r->fnt_pnt_size;
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

