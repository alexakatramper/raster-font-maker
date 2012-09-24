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

using std::map;

struct CharInfo
{
	FT_UInt	charcode;
	int		x;
	int		y;
	int		width;
	int		height;
	int		xoffset;
	int		yoffset;
	int		xadvance;
	int		page;
	FT_Glyph	glyph;
	
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
};


typedef map<FT_UInt,CharInfo> CharSet;
typedef CharSet::iterator	CharSetIt;

class FontMaker;

struct Pixel
{
//	unsigned char a;
//	unsigned char b;
//	unsigned char g;
//	unsigned char r;
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;

	Pixel() : a(0), b(0), g(0), r(0) {}
	Pixel( int c ) { a = c >> 24; b = ( ( c >> 16 ) & 0xFF ); g = ( ( c >> 8 ) & 0xFF ); r = ( c & 0xFF ); }

};


struct SpanFuncParams
{
	FontMaker*	maker;
	Pixel*		buf;	// abgr
	int			yOffset;
	Pixel		color;
	bool		outline;
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
	
	void setFontColor( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void setOutlineColor( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	
	void setOutlineWidth( float w ) { _outlineWidth = w; }
	
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
	Pixel		_fontColor;
	Pixel		_outlineColor;
//	vector<int*>		_pages;
	
	int			_flags;
};
