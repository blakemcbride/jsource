/* Copyright 1990-2006, Jsoftware Inc.  All rights reserved.               */
/* Licensed use only. Any other use is in violation of copyright.          */
/*                                                                         */
/* Debug: Suspension                                                       */

#include "j.h"
#include "d.h"
#include "w.h"


/* deba() and debz() must be coded and executed in pairs */
/* in particular, do NOT do error exits between them     */
/* e.g. the following is a NO NO:                        */
/*    d=deba(...);                                       */
/*    ASSERT(blah,EVDOMAIN);                             */
/*    debz()                                             */
// q, if given, is the block to use for the debug stack; otherwise allocated

DC jtdeba(J jt,C t,A x,A y,A fs,DC d){
 if(!d){A q; GATV(q,LIT,sizeof(DST),1,0); d=(DC)AV(q);}
 memset(d,C0,sizeof(DST));
 d->dctype=t; d->dclnk=jt->sitop; jt->sitop=d;
 switch(t){
  case DCPARSE:  d->dcy=(A)AAV(y); d->dcn=AN(y); break;
  case DCSCRIPT: d->dcy=y; d->dcm=(I)fs; break;
  case DCCALL:   
   d->dcx=x; d->dcy=y; d->dcf=fs; 
   d->dca=jt->curname; d->dcm=NAV(jt->curname)->m;
   d->dcn=(I)jt->cursymb;
   d->dcstop=-2;
   if(jt->dbss==SSSTEPINTO){d->dcss=SSSTEPINTO; jt->dbssd=d; jt->dbss=0;}
 }
 R d;
}    /* create new top of si stack */

void jtdebz(J jt){jt->sitop=jt->sitop->dclnk;}
     /* remove     top of si stack */

F1(jtsiinfo){A z,*zv;DC d;I c=5,n,*s;
 ASSERTMTV(w);
 n=0; d=jt->sitop; while(d){++n; d=d->dclnk;}
 GATV(z,BOX,c*n,2,0); s=AS(z); s[0]=n; s[1]=c; zv=AAV(z);
 d=jt->sitop;
 while(d){
  RZ(zv[0]=sc(d->dctype));
  RZ(zv[1]=d->dcsusp?scc('*'):scc(' '));
  RZ(zv[2]=sc((I)d->dcss));
  RZ(zv[3]=d->dctype==DCCALL?sc(lnumsi(d)):mtv);
  switch(d->dctype){
   case DCPARSE:  RZ(zv[4]=unparse(d->dcy)); break;
   case DCCALL:   RZ(zv[4]=sfn(0,d->dca));   break;
   case DCSCRIPT: zv[4]=d->dcy;              break;
   case DCJUNK:   zv[4]=mtv;                 break; 
  }
  zv+=c; d=d->dclnk;
 }
 R z;
}    /* 13!:32 si info */

I lnumcw(I j,A w){CW*u;
 if(0>j)R -2; 
 else if(!w)R j; 
 else{u=(CW*)AV(w); DO(AN(w), if(j<=u[i].source)R i;) R IMAX/2;}
}    /* line number in CW corresp. to j */

I lnumsi(DC d){A c;I i;
 if(c=d->dcc){i=*(I*)d->dci; R(MIN(i,AN(c)-1)+(CW*)AV(c))->source;}else R 0;
}    /* source line number from stack entry */



static DC suspset(DC d){DC e;
 while(d&&DCCALL!=d->dctype){e=d; d=d->dclnk;}  /* find topmost call                 */
 if(!(d&&DCCALL==d->dctype))R 0;                /* don't suspend if no such call     */
 if(d->dcc)e->dcsusp=1;                         /* if explicit, set susp on line     */
 else      d->dcsusp=1;                         /* if not explicit, set susp on call */
 R d;
}    /* find topmost call and set suspension flag */

static B jterrcap(J jt){A y,*yv;
 jt->dbsusact=SUSCLEAR;
 GAT(y,BOX,4,1,0); yv=AAV(y);
 RZ(yv[0]=sc(jt->jerr1));
 RZ(yv[1]=str(jt->etxn1,jt->etx));
 RZ(yv[2]=dbcall(mtv));
 RZ(yv[3]=locname(mtv));
 RZ(symbis(nfs(22L,"STACK_ERROR_INFO_base_"),y,mark));
 R 1;
}    /* error capture */

static void jtsusp(J jt){B t;DC d;I old=jt->tnextpushx;
 jt->dbsusact=SUSCONT;
 d=jt->dcs; t=jt->tostdout;
 jt->dcs=0; jt->tostdout=1;
 jt->fdepn =MIN(NFDEP ,jt->fdepn +NFDEP /10);
 jt->fcalln=MIN(NFCALL,jt->fcalln+NFCALL/10);
 if     (jt->dbssexec){RESETERR; immex(jt->dbssexec); tpop(old);}
 else if(jt->dbtrap  ){RESETERR; immex(jt->dbtrap  ); tpop(old);}
 while(jt->dbsusact==SUSCONT){
  jt->jerr=0;
  if(jt->iepdo&&jt->iep){jt->iepdo=0; immex(jt->iep); tpop(old);}
  immex(jgets("      ")); 
  tpop(old);
 }
 if(jt->dbuser){jt->fdepn-=NFDEP/10; jt->fcalln-=NFCALL/10;}
 else          {jt->fdepn =NFDEP;    jt->fcalln =NFCALL;   }
 jt->dcs=d; jt->tostdout=t;
}    /* user keyboard loop while suspended */

static A jtdebug(J jt){A z=0;C e;DC c,d;I*v;
 if(jt->dbssd){jt->dbssd->dcss=0; jt->dbssd=0;}
 RZ(d=suspset(jt->sitop));
 v=(I*)d->dci; 
 if(0>*v)R 0;
 e=jt->jerr; jt->jerr=0;
 if(DBERRCAP==jt->db)errcap(); else susp();
 switch(jt->dbsusact){
  case SUSRUN:      
   --*v; break;
  case SUSRET:      
   *v=-2; z=jt->dbresult; jt->dbresult=0; break;
  case SUSJUMP: 
   *v=lnumcw(jt->dbjump,d->dcc)-1; break;
  case SUSCLEAR:
   jt->jerr=e;    
   c=jt->sitop; 
   while(c){if(DCCALL==c->dctype)*(I*)(c->dci)=-2; c=c->dclnk;} 
 }
 if(jt->dbsusact!=SUSCLEAR)jt->dbsusact=SUSCONT;
 d->dcsusp=0;
 R z;
}


// Parse line & check for assertion
static A jtparseas(J jt,B as,A w){A z;
 // If w is 0, we don't parse, but instead raise 'noun result was required' error on the beginning of the sentence
 if(!w){jt->sitop->dci=1; jsignal(EVNONNOUN); R 0;}
 // normal path is to execute the sentence
 z=parsea(w);  /* y is destroyed by parsea ??? */
 if(as&&z)ASSERT(NOUN&AT(z)&&all1(eq(one,z)),EVASSERT);
 R z;
}

/* parsex: parse an explicit defn line              */
/* w  - line to be parsed                           */
/* lk - 1 iff locked function; _1 to signal noun error at beginning of sentence */
/* ci - current row of control matrix               */
/* c  - stack entry for dbunquote for this function */
// d - DC area to use in deba

A jtparsex(J jt,A w,B lk,CW*ci,DC c,DC d){A z;B as,s;DC t=jt->sitop;
 RZ(w);
 JATTN;
 as=ci->type==CASSERT;
 if((signed char)lk>0)R parseas(as,w);
 RZ(d=deba(DCPARSE,0L,w,0L,d));
 if((signed char)lk<0)w=0;  // If 'signal noun error' mode, pull the rug from under parse
 if(0==c)z=parseas(as,w);   /* anonymous or not debug */
 else{                      /* named and debug        */
  if(s=dbstop(c,ci->source)){z=0; jsignal(EVSTOP);}
  else                      {z=parseas(as,w);     }
  // If we hit a stop, or if we hit an error outside of try./catch., enter debug mode.  But if debug mode is off now, we must have just
  // executed 13!:0]0, and we should continue on outside of debug mode
  if(!z&&jt->db&&(s||DBTRY!=jt->db)){t->dcj=d->dcj=jt->jerr; z=debug(); t->dcj=0;} //  d is PARSE type; set d->dcj=err#; d->dcn must remain # tokens
 }
 debz();
 R z;
}

DF2(jtdbunquote){A t,z;B b=0,s;DC d;I i;V*sv;
 sv=VAV(self); t=sv->f; 
 RZ(d=deba(DCCALL,a,w,self,0L));
 if(CCOLON==sv->id&&t&&NOUN&AT(t)){  /* explicit */
  ras(self); z=a?dfs2(a,w,self):dfs1(w,self); fa(self);
 }else{                              /* tacit    */
  i=0; d->dci=(I)&i;
  while(0==i){
   if(s=dbstop(d,0L)){z=0; jsignal(EVSTOP);}
   else              {ras(self); z=a?dfs2(a,w,self):dfs1(w,self); fa(self);}
   // If we hit a stop, or if we hit an error outside of try./catch., enter debug mode.  But if debug mode is off now, we must have just
   // executed 13!:8]0, and we should continue on outwide of debug mode
   if(!z&&jt->db&&(s||DBTRY!=jt->db)){d->dcj=jt->jerr; z=debug(); if(self!=jt->sitop->dcf)self=jt->sitop->dcf;}
   if(b){fa(a); fa(w);}
   if(b=jt->dbalpha||jt->dbomega){a=jt->dbalpha; w=jt->dbomega; jt->dbalpha=jt->dbomega=0;}
   ++i;
  }
 }
 if(d->dcss)ssnext(d,d->dcss);
 if(jt->dbss==SSSTEPINTOs)jt->dbss=0;
 debz();
 R z;
}    /* function call, debug version */


F1(jtdbc){UC k;
 RZ(w);
 if(AN(w)){
  RE(k=(UC)i0(w));
  ASSERT(!k||k==DB1||k==DBERRCAP,EVDOMAIN);
  ASSERT(!k||!jt->glock,EVDOMAIN);
 }
 jt->redefined=0;
 if(AN(w)){jt->db=jt->dbuser=k; jt->fdepn=NFDEP/(k?2:1); jt->fcalln=NFCALL/(k?2:1);}
 jt->dbsusact=SUSCLEAR; 
 R mtm;
}    /* 13!:0  clear stack; enable/disable suspension */

F1(jtdbq){ASSERTMTV(w); R sc(jt->dbuser);}
     /* 13!:17 debug flag */

F1(jtdbrun ){ASSERTMTV(w); jt->dbsusact=SUSRUN;  R mtm;}
     /* 13!:4  run again */

F1(jtdbnext){ASSERTMTV(w); jt->dbsusact=SUSNEXT; R mtm;}
     /* 13!:5  run next */

F1(jtdbret ){RZ(w); jt->dbsusact=SUSRET; ras(w); jt->dbresult=w; R mtm;}
     /* 13!:6  exit with result */

F1(jtdbjump){RE(jt->dbjump=i0(w)); jt->dbsusact=SUSJUMP; R mtm;}
     /* 13!:7  resume at line n (return result error if out of range) */

static F2(jtdbrr){DC d;
 RE(0);
 d=jt->sitop; while(d&&DCCALL!=d->dctype)d=d->dclnk; 
 ASSERT(d&&VERB&AT(d->dcf)&&!d->dcc,EVDOMAIN);  /* must be explicit verb */
 ras(a); jt->dbalpha=a; ras(w); jt->dbomega=w; 
 jt->dbsusact=SUSRUN;
 R mtm;
}

F1(jtdbrr1 ){R dbrr(0L,w);}   /* 13!:9   re-run with arg(s) */
F2(jtdbrr2 ){R dbrr(a, w);}

F1(jtdbtrapq){ASSERTMTV(w); R jt->dbtrap?jt->dbtrap:mtv;}   
     /* 13!:14 query trap */

F1(jtdbtraps){RZ(w=vs(w)); fa(jt->dbtrap); if(AN(w)){ras(w); jt->dbtrap=w;}else jt->dbtrap=0L; R mtm;}
     /* 13!:15 set trap */
