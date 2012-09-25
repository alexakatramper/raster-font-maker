//
//  FontMaker.h
//  RasterFontMaker
//
//  Created by Alexey Schutsky on 9/17/12.
//  Copyright (c) 2012 Alexey Schutsky. All rights reserved.
//

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <map>
#include <vector>

using std::map;
using std::vector;

struct Span
{
	Span() { }
	Span(int _x, int _y, int _width, int _coverage)
	: x(_x), y(_y), width(_width), coverage(_coverage) { }
	
	int x, y, width, coverage;
};

typedef vector<Span> Spans;



struct CharInfo
{
	FT_UInt		charcode;
	int			x;
	int			y;
	int			xMin;
	int			xMax;
	int			yMin;
	int			yMax;
	int			width;
	int			height;
	int			xoffset;
	int			yoffset;
	int			xadvance;
	int			page;
	FT_Glyph	glyph;
	Spans		outlineSpans;
	Spans		bodySpans;
	
	CharInfo(): charcode(0), x(0), y(0), xoffset(0), yoffset(0), xadvance(0), page(0), glyph(0) {}
	
	CharInfo( const CharInfo& ci ):  charcode(ci.charcode), x(ci.x), y(ci.y), xoffset(ci.xoffset), yoffset(ci.yoffset), xadvance(ci.xadvance), page(ci.page), glyph(0)
	{
		if( ci.glyph )
			FT_Glyph_Copy( ci.glyph, &glyph );
	}
	
	~CharInfo()
	{
//		if( glyph )
//			FT_Done_Glyph( glyph ); // TODO: fix crash here!
	}
	
	static bool compareByHeight( const CharInfo& i, const CharInfo& j ) { return ( i.height < j.height ); }
	
	void updateSize()
	{
		width = 0;
		height = 0;
		xMin = 0;
		xMax = 0;
		yMin = 0;
		yMax = 0;
		
		for( size_t i = 0; i < bodySpans.size(); i++ )
		{
			if( xMin > bodySpans[i].x )
				xMin = bodySpans[i].x;
			if( xMax < bodySpans[i].x + bodySpans[i].width )
				xMax = bodySpans[i].x + bodySpans[i].width;
			if( yMin > bodySpans[i].y )
				yMin = bodySpans[i].y;
			if( yMax < bodySpans[i].y )
				yMax = bodySpans[i].y;
		}
		
		for( size_t i = 0; i < outlineSpans.size(); i++ )
		{
			if( xMin > outlineSpans[i].x )
				xMin = outlineSpans[i].x;
			if( xMax < outlineSpans[i].x + outlineSpans[i].width )
				xMax = outlineSpans[i].x + outlineSpans[i].width;
			if( yMin > outlineSpans[i].y )
				yMin = outlineSpans[i].y;
			if( yMax < outlineSpans[i].y )
				yMax = outlineSpans[i].y;
		}

		width = xMax - xMin;
		height = yMax - yMin;
	}
};



typedef map<FT_UInt,CharInfo> CharSet;
typedef CharSet::iterator	CharSetIt;

class FontMaker;


struct PixelData32
{
	PixelData32(): r(0), g(0), b(0), a(255) {}
	PixelData32( unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_ = 255 ) : r(r_), g(g_), b(b_), a(a_) {}
	PixelData32( int c ) { a = c >> 24; b = ( ( c >> 16 ) & 0xFF ); g = ( ( c >> 8 ) & 0xFF ); r = ( c & 0xFF ); }
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};


struct SpanFuncParams
{
	FontMaker*		maker;
	PixelData32*	buf;
	int				yOffset;
	PixelData32		color;
	bool			outline;
};


class FontMaker
{
private:
	FontMaker();
	FontMaker( const FontMaker& ){}
	FontMaker( const FontMaker* ){}
	
	~FontMaker();
	
	static FontMaker* _instance;
	
public:
	static FontMaker* instance()
	{
		if( _instance == 0 )
			_instance = new FontMaker();
		return _instance;
	}
	
	void cleanup();
	void loadFont( const char* fileName );
	void setFontSize( int size );
	int fontSize() { return _fontSize; }
	
	void setImageSize( int width, int height );
	void setImageWidth( int width ) { _imageWidth = width; }
	void setImageHeight( int height ) { _imageHeight = height; }

	int imageWidth() { return _imageWidth; }
	int imageHeight() { return _imageHeight; }

	
	void resetCharSet();
	
	void setCharList( const char* chars );	// TODO: remake to UTF-16
	void addChar( FT_UInt ch );
	void addCharRange( FT_UInt ch1, FT_UInt ch2 );	// TODO: remake to UTF-16, remake to 'addCharRange'
	void removeCharRange( FT_UInt ch1, FT_UInt ch2 );	// TODO: remake to UTF-16, remake to 'addCharRange'
	
	int makeLayout();
	
	void drawPage( int page, int* abgr );

	void exportXML( const char* fileName, const char* path );
	void exportTXT( const char* fileName, const char* path );

	void setDrawFrames( bool state );
	void setDrawOutline( bool state );

	void setPadding( int value ) { _padding = value; }
	
	const char* fontName();
	
	static void spanFunc( int y, int count, const FT_Span* spans, void* user );
	static void renderCallback( int y, int count, const FT_Span* spans, void* user );
	
	void setFontColor( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void setOutlineColor( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	
	void setOutlineWidth( float w ) { _outlineWidth = w; }
	
	bool fontLoaded() { return ( _face != 0 ); }
	
	// new stuff
	void strokeChars();
	int layoutChars();
	void drawChars( int page, PixelData32* buf );
	
private:
	FT_Library	_library;
	FT_Face		_face;
	
	int			_fontSize;
	int			_padding;
	int			_pageCount;
	int			_lineHeight;
	int			_imageWidth;
	int			_imageHeight;
	CharSet		_charSet;
	float		_outlineWidth;
	PixelData32	_fontColor;
	PixelData32	_outlineColor;
	
	int			_flags;
};
