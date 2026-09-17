#pragma once
// Offline layout adapter. Uses the production draw functions, without a window server.
#include <codecvt>
#include <locale>
#include <iostream>
using HDC=int;using HWND=int;using HFONT=int;using COLORREF=unsigned;using UINT=unsigned;
struct RECT{int left,top,right,bottom;};
constexpr unsigned RGB(int r,int g,int b){return unsigned(r|(g<<8)|(b<<16));}
constexpr UINT DT_LEFT=0,DT_TOP=0,DT_CENTER=1,DT_RIGHT=2,DT_VCENTER=4,DT_SINGLELINE=32,DT_WORDBREAK=16;
extern HFONT smallFont,normalFont,boldFont,titleFont,numberFont;
extern const COLORREF ink;
inline std::ostringstream svg;
inline int clipId=0;
inline std::string utf8(const std::wstring&s){return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(s);}
inline std::string xml(const std::wstring&s){std::string r;for(char c:utf8(s)){if(c=='&')r+="&amp;";else if(c=='<')r+="&lt;";else if(c=='>')r+="&gt;";else if(c=='\"')r+="&quot;";else r+=c;}return r;}
inline std::string color(COLORREF c){std::ostringstream o;o<<"rgb("<<(c&255)<<","<<((c>>8)&255)<<","<<((c>>16)&255)<<")";return o.str();}
inline void textAt(HDC,int x,int y,int w,int h,const std::wstring&s,HFONT f=normalFont,COLORREF c=ink,UINT flags=DT_LEFT|DT_TOP|DT_WORDBREAK){
 int size=f%1000,lineH=int(size*1.2);bool single=flags&DT_SINGLELINE;std::vector<std::wstring>lines;std::wstring line;double length=0;
 for(wchar_t ch:s){double advance=ch<128?size*.54:size;
  if(ch=='\n'){lines.push_back(line);line.clear();length=0;continue;}
  if(!single&&length+advance>w&&!line.empty()){lines.push_back(line);line.clear();length=0;}
  line+=ch;length+=advance;
 }lines.push_back(line);
 int id=clipId++;svg<<"<clipPath id='c"<<id<<"'><rect x='"<<x<<"' y='"<<y<<"' width='"<<w<<"' height='"<<h<<"'/></clipPath><g clip-path='url(#c"<<id<<")'>";
 int yy=y+size;if(flags&DT_VCENTER)yy=y+(h-size)/2+size-2;
 for(const auto&v:lines){int xx=x;const char* anchor="start";if(flags&DT_CENTER){xx=x+w/2;anchor="middle";}else if(flags&DT_RIGHT){xx=x+w;anchor="end";}
  svg<<"<text x='"<<xx<<"' y='"<<yy<<"' text-anchor='"<<anchor<<"' font-family='Noto Sans CJK SC' font-size='"<<size<<"' font-weight='"<<(f>=1000?600:400)<<"' fill='"<<color(c)<<"'>"<<xml(v)<<"</text>";yy+=lineH;}
 svg<<"</g>";
}
inline void box(HDC,RECT r,COLORREF fill,COLORREF border,int radius=14){svg<<"<rect x='"<<r.left<<"' y='"<<r.top<<"' width='"<<r.right-r.left<<"' height='"<<r.bottom-r.top<<"' rx='"<<radius/2<<"' fill='"<<color(fill)<<"' stroke='"<<color(border)<<"'/>";}
inline void line(HDC,int x,int y,int x2,int y2,COLORREF c,int width=1){svg<<"<line x1='"<<x<<"' y1='"<<y<<"' x2='"<<x2<<"' y2='"<<y2<<"' stroke='"<<color(c)<<"' stroke-width='"<<width<<"'/>";}
void drawArt(HDC,int i,int x,int y,int w,int h);
