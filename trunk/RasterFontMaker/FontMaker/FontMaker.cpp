//
//  FontMaker.cpp
//  RasterFontMaker
//
//  Created by Alexey Schutsky on 9/17/12.
//  Copyright (c) 2012 Alexey Schutsky. All rights reserved.
//

#include "FontMaker.h"
#include "xmlparser.h"

#include <assert.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

FontMaker* FontMaker::_instance = 0;


#define DRAW_BBOX	1

//---------------------------------------------------------------------------------
//	FontMaker()
//---------------------------------------------------------------------------------
FontMaker::FontMaker() : _library(0), _face(0), _flags(DRAW_BBOX), _fontSize(0), _pageCount(0), _lineHeight(0)
{
	_imageHeight = 512;
	_imageWidth = 512;
	
	int error = FT_Init_FreeType( &_library );
	if ( error )
	{
		printf( "ERROR: can't init library\n" );
	}
}


//---------------------------------------------------------------------------------
//	~FontMaker()
//---------------------------------------------------------------------------------
FontMaker::~FontMaker()
{
}


//---------------------------------------------------------------------------------
//	cleanup()
//---------------------------------------------------------------------------------
void FontMaker::cleanup()
{
	if( _face )
		FT_Done_Face( _face );
	_face = 0;
}


//---------------------------------------------------------------------------------
//	loadFont()
//---------------------------------------------------------------------------------
void FontMaker::loadFont( const char* fileName )
{
	cleanup();
	
	int error = FT_New_Face( _library, fileName, 0, &_face );
	if( error == FT_Err_Unknown_File_Format )
	{
		printf( "ERROR: unsupported file format\n" );
		return;
	}
	else if( error )
	{
		printf( "ERROR: can't load font, error# %i\n", error );
		return;
	}
	
	printf( "OK: font loaded\n" );
	printf( "- name:		%s\n", _face->family_name );
	printf( "- style:		%s\n", _face->style_name );	
	printf( "- faces:		%li\n", _face->num_faces );
	printf( "- charmaps:	%i\n", _face->num_charmaps );
	printf( "- platform_id:	%i\n", _face->charmap->platform_id );
	printf( "- encoding_id:	%i\n", _face->charmap->encoding_id );
	printf( "- glyps:		%li\n", _face->num_glyphs );
	
	
	
//	FT_ULong  charcode;
//	FT_UInt   gindex;
//	
//	charcode = FT_Get_First_Char( _face, &gindex );
//	while ( gindex != 0 )
//	{
//		printf( "charcode = 0x%08lX		index = %i\n", charcode, gindex );
//		charcode = FT_Get_Next_Char( _face, charcode, &gindex );
//	}
	
}


//---------------------------------------------------------------------------------
//	setFontSize()
//---------------------------------------------------------------------------------
void FontMaker::setFontSize( int size )
{
	assert( _face );
	_fontSize = size;
	FT_Set_Char_Size( _face, 0, size * 64, 72, 72 );
}


//---------------------------------------------------------------------------------
//	setImageSize()
//---------------------------------------------------------------------------------
void FontMaker::setImageSize( int width, int height )
{
	_imageWidth = width;
	_imageHeight = height;
}


//---------------------------------------------------------------------------------
//	resetCharSet()
//---------------------------------------------------------------------------------
void FontMaker::resetCharSet()
{
	_charSet.clear();
}


//---------------------------------------------------------------------------------
//	setCharList()
//---------------------------------------------------------------------------------
void FontMaker::setCharList( const char* chars )
{
	resetCharSet();
	
	// TODO: remake to UTF-16
	size_t count = strlen( chars );
	
	for( size_t i = 0; i < count; i++ )
	{
		CharInfo ci;
		ci.charcode = chars[i];
		_charSet[ chars[i] ] = ci;
	}
}


//---------------------------------------------------------------------------------
//	addChar()
//---------------------------------------------------------------------------------
void FontMaker::addChar( FT_UInt ch )
{
	CharInfo ci;
	ci.charcode = ch;
	_charSet[ ch ] = ci;
}

//---------------------------------------------------------------------------------
//	addCharRange()
//---------------------------------------------------------------------------------
void FontMaker::addCharRange( FT_UInt ch1, FT_UInt ch2 )
{
	for( FT_UInt ch = ch1; ch <= ch2; ch++ )
		addChar( ch );
}


//---------------------------------------------------------------------------------
//	removeCharRange()
//---------------------------------------------------------------------------------
void FontMaker::removeCharRange( FT_UInt ch1, FT_UInt ch2 )
{
	for( FT_UInt ch = ch1; ch <= ch2; ch++ )
	{
		_charSet.erase( ch );
	}
}


//---------------------------------------------------------------------------------
//	makeLayout()
//---------------------------------------------------------------------------------
int FontMaker::makeLayout()
{
	// 1.load glyphs & info
//	FT_BBox bbox;
	CharInfo* ci = 0;
	
	for( CharSetIt it = _charSet.begin(); it != _charSet.end(); it++ )
	{
		ci = &(*it).second;
		FT_UInt glyph_index = FT_Get_Char_Index( _face, ci->charcode );
		
		if( glyph_index == 0 )
			printf( "WARNING: no glyph for charcode 0x%X\n", ci->charcode );

		FT_Load_Glyph( _face, glyph_index, FT_LOAD_DEFAULT );
		FT_Get_Glyph( _face->glyph, &ci->glyph );
//		FT_Glyph_Get_CBox( ci->glyph, FT_GLYPH_BBOX_TRUNCATE, &bbox );
		
		ci->width = _face->glyph->metrics.width / 64 + _padding * 2; //bbox.xMax - bbox.xMin;
		ci->height = _face->glyph->metrics.height / 64 + _padding * 2; //bbox.yMax - bbox.yMin;
		
		ci->xoffset = 0;
//		ci->yoffset = ( _face->glyph->metrics.height - _face->glyph->metrics.horiBearingY ) / 64;
//		ci->yoffset = ( _face->glyph->metrics.horiBearingY - _face->glyph->metrics.height ) / 64;
//		ci->yoffset = _face->glyph->metrics.horiBearingY / 64;

		ci->yoffset = ( _face->ascender - _face->glyph->metrics.horiBearingY - _face->descender ) / 64;
		
		

		ci->xadvance = _face->glyph->metrics.horiAdvance / 64;
	}
	
	// 2. set positions - arrange by rows & colums
	// TODO: apply padding
	_pageCount = 0;
	int x = 0;
	int y = 0;
	int maxHeight = 0;
	
	_lineHeight = 0;
	
	vector<CharInfo*> charLine;
	
	for( CharSetIt it = _charSet.begin(); it != _charSet.end(); it++ )
	{
		ci = &(*it).second;
		
		if( ( x + ci->width ) > _imageWidth )
		{
			// put chars of current line into vector
			// when the line is done - check if glyphs are not exeeded the page's height
			// and move this line to new page if needed
						
			for( size_t m = 0; m < charLine.size(); m++ )
			{
				if( ( charLine[m]->y + charLine[m]->height * 2 ) >= _imageHeight )
				{
					++_pageCount;
					y = 0;
					// move this line to new page
					for( size_t k = 0; k < charLine.size(); k++ )
					{
						charLine[k]->page = _pageCount;
						charLine[k]->y = 0;
					}
					break;
				}
			}

			// start new line
			x = 0;
			y += maxHeight;
			maxHeight = 0;
			charLine.clear();
		}
		
		ci->page = _pageCount;
		ci->x = x;
		ci->y = y;
		
		x += ci->width;
		
		if( maxHeight < ci->height )
			maxHeight = ci->height;
		if( _lineHeight < ci->height )
			_lineHeight = ci->height;
		
		charLine.push_back( ci );
	}
	
	// check the last line if it is not out of page bottom
	for( size_t j = 0; j < charLine.size(); j++ )
	{
		if( ( charLine[j]->y + charLine[j]->height ) >= _imageHeight )
		{
			++_pageCount;
			// move this line to new page
			for( size_t k = 0; k < charLine.size(); k++ )
			{
				charLine[k]->page = _pageCount;
				charLine[k]->y = 0;
			}
			break;
		}
	}

	
	++_pageCount;
	
	return _pageCount;
}


//---------------------------------------------------------------------------------
//	drawPage()
//---------------------------------------------------------------------------------
void FontMaker::drawPage( int page, int* abgr, int color )
{
	// clear image
	memset( abgr, 0, _imageHeight * _imageWidth * sizeof( int ) );

//	// for debug: fill background
//	int* ptr = abgr;
//	for( size_t i = 0; i < _imageHeight * _imageWidth; i++ )
//	{
//		*ptr = 0xFFFF0000;
//		++ptr;
//	}

	
	CharInfo* ci = 0;
	for( CharSetIt it = _charSet.begin(); it != _charSet.end(); it++ )
	{
		ci = &(*it).second;
		
		if( ci->page != page )
			continue;
		

		if( ci->glyph->format != FT_GLYPH_FORMAT_BITMAP )
		{
			FT_Glyph_To_Bitmap( &ci->glyph, FT_RENDER_MODE_NORMAL, 0, 1 );
		}
		
		FT_BitmapGlyph glyph_bitmap = (FT_BitmapGlyph)ci->glyph;
		
		// TODO: take xOffset/yOffset from glyph_bitmap
		
		int* dstPtr = &abgr[ ci->x + _padding + ( ci->y + _padding ) * _imageWidth ];
		
				
		// copy raster data
		FT_Bitmap bitmap = glyph_bitmap->bitmap;
		
		for( int y = 0; y < bitmap.rows; y++ )
		{
			int n = ci->x + _padding + ( ci->y + _padding + y ) * _imageWidth;
			
			dstPtr = &abgr[ n ];
			for( int x = 0; x < bitmap.width; x++ )
			{
				if( bitmap.buffer [ x + y * bitmap.width ] != 0 )
				{
					*dstPtr = bitmap.buffer [ x + y * bitmap.width ] << 24;
					
					*dstPtr |= ( 0x00FFFFFF & color );
				}
				++dstPtr;
			}
		}
		
		if( _flags & DRAW_BBOX )
		{
			// top line
			dstPtr = &abgr[ ci->x + ci->y * _imageWidth ];
			for( int x = 0; x < ci->width; x++ )
			{
				*dstPtr = 0xFF0000FF;
				++dstPtr;
			}
			// bottom line
			dstPtr = &abgr[ ci->x + ( ci->y + ci->height - 1 ) * _imageWidth ];
			for( int x = 0; x < ci->width; x++ )
			{
				*dstPtr = 0xFF0000FF;
				++dstPtr;
			}
			// left line
			dstPtr = &abgr[ ci->x + ci->y * _imageWidth ];
			for( int y = 0; y < ci->height; y++ )
			{
				*dstPtr = 0xFF0000FF;
				dstPtr += _imageWidth;
			}
			// right line
			dstPtr = &abgr[ ci->x + ci->width - 1 + ci->y * _imageWidth ];
			for( int y = 0; y < ci->height; y++ )
			{
				*dstPtr = 0xFF0000FF;
				dstPtr += _imageWidth;
			}

		}

	}

}


//------------------------------------------------------------------------------
//	_addIntAttribute
//------------------------------------------------------------------------------
void _addIntAttribute( XMLNode& parent, const char* key, int value )
{
	char tmpStr[32] = { 0 };
	
	sprintf( tmpStr, "%i", value );
	parent.addAttribute( key, tmpStr );
}


//---------------------------------------------------------------------------------
//	exportXML()
//---------------------------------------------------------------------------------
void FontMaker::exportXML( const char* fileName, const char* path )
{
	string fullName( path );
	fullName += "/";
	fullName += fileName;
	fullName += ".fnt";

	char tmpStr[256] = {0};

	XMLNode::setGlobalOptions( 1, 1, 1 );

//	XMLNode xRoot = XMLNode::createXMLTopNode( "xml" );
//	XMLNode xFont = xRoot.addChild( "font" );

	XMLNode xFont = XMLNode::createXMLTopNode( "font" );

//	<info face="Comic Sans MS" size="72" bold="0" italic="0" charset="" unicode="1" stretchH="100" smooth="1" aa="1" padding="10,10,10,10" spacing="10,10" outline="0"/>
	XMLNode xInfo = xFont.addChild( "info" );

	xInfo.addAttribute( "face", _face->family_name );
	_addIntAttribute( xInfo, "size", _fontSize );
	xInfo.addAttribute( "bold", "0" );
	xInfo.addAttribute( "italic", "0" );
	xInfo.addAttribute( "charset", "" );
	xInfo.addAttribute( "unicode", "1" );
	xInfo.addAttribute( "stretchH", "100" );
	xInfo.addAttribute( "smooth", "1" );
	xInfo.addAttribute( "aa", "1" );
	sprintf( tmpStr, "%i,%i,%i,%i", _padding, _padding, _padding, _padding );
	xInfo.addAttribute( "padding", tmpStr );
	xInfo.addAttribute( "spacing", "0,0" );
	xInfo.addAttribute( "outline", "0" );
	

	//		<common lineHeight="72" base="57" scaleW="1024" scaleH="2048" pages="1" packed="0" alphaChnl="1" redChnl="0" greenChnl="0" blueChnl="0"/>
	XMLNode xCommon = xFont.addChild( "common" );
//	_addIntAttribute( xCommon, "lineHeight", _lineHeight );
//	_addIntAttribute( xCommon, "lineHeight", _face->ascender + _face->descender );
	_addIntAttribute( xCommon, "lineHeight", _face->size->metrics.height / 64 );
	_addIntAttribute( xCommon, "base", _face->size->metrics.ascender / 64 );
	_addIntAttribute( xCommon, "scaleW", _imageWidth );
	_addIntAttribute( xCommon, "scaleH", _imageHeight );
	_addIntAttribute( xCommon, "pages", _pageCount );
	xCommon.addAttribute( "packed", "0" );
	xCommon.addAttribute( "alphaChnl", "1" );
	xCommon.addAttribute( "redChnl", "0" );
	xCommon.addAttribute( "greenChnl", "0" );
	xCommon.addAttribute( "blueChnl", "0" );

	
	//		<page id="0" file="Main_hd.png" />
	XMLNode xPages = xFont.addChild( "pages" );
	for( int p = 0; p < _pageCount; p++ )
	{
		XMLNode xPage = xPages.addChild( "page" );
		_addIntAttribute( xPage, "id", p );
		
		string pageName( fileName );
		if( _pageCount == 1 )
			pageName.append( ".png" );
		else
		{
			char ext[32] = {0};
			sprintf( ext, "_%i.png", p );
			pageName.append( ext );
		}
		
		xPage.addAttribute( "file", pageName.c_str() );
	}
	
//	<chars count="432">
	XMLNode xChars = xFont.addChild( "chars" );
	_addIntAttribute( xChars, "count", (int)_charSet.size() );

//		<char id="32" x="993" y="966" width="21" height="21" xoffset="-10" yoffset="47" xadvance="15" page="0" chnl="15" />
	CharInfo* ci = 0;
	for( CharSetIt it = _charSet.begin(); it != _charSet.end(); it++ )
	{
		ci = &(*it).second;
		XMLNode xChar = xChars.addChild( "char" );
		_addIntAttribute( xChar, "id", ci->charcode );
		_addIntAttribute( xChar, "x", ci->x );
		_addIntAttribute( xChar, "y", ci->y );
		_addIntAttribute( xChar, "width", ci->width );
		_addIntAttribute( xChar, "height", ci->height );
		_addIntAttribute( xChar, "xoffset", ci->xoffset );
		_addIntAttribute( xChar, "yoffset", ci->yoffset );
		_addIntAttribute( xChar, "xadvance", ci->xadvance );
		_addIntAttribute( xChar, "page", ci->page );
		xChar.addAttribute( "chnl", "15" );
	}

	xFont.writeToFile( fullName.c_str() );
}


//---------------------------------------------------------------------------------
//	exportTXT()
//---------------------------------------------------------------------------------
void FontMaker::exportTXT( const char* fileName, const char* path )
{
	string fullName( path );
	fullName += "/";
	fullName += fileName;
	fullName += ".fnt";
	
	FILE* f = fopen( fullName.c_str(), "w" );

	fprintf( f, "info face=\"TestFont\" size=%i bold=0 italic=0 charset=\"\" unicode=1 stretchH=100 smooth=1 aa=3 padding=%i,%i,%i,%i spacing=0,0 outline=0\n",
			_fontSize, _padding, _padding, _padding, _padding );
	
	fprintf( f, "common lineHeight=%i base=15 scaleW=%i scaleH=%i pages=%i packed=0 alphaChnl=1 redChnl=0 greenChnl=0 blueChnl=0\n",
			_lineHeight, _imageWidth, _imageHeight, _pageCount );
	
	if( _pageCount == 1 )
	{
		fprintf( f, "page id=0 file=\"%s.png\"\n", fileName );
	}
	else
	{
		for( int p = 0; p < _pageCount; p++ )
		{
			fprintf( f, "page id=%i file=\"%s_%i.png\"\n", p, fileName, p );
		}
	}

	fprintf( f, "chars count=%li\n", _charSet.size() );
	

	CharInfo* ci = 0;
	for( CharSetIt it = _charSet.begin(); it != _charSet.end(); it++ )
	{
		ci = &(*it).second;
		fprintf( f, "char id=%-5u x=%-4i y=%-4i width=%-4i height=%-4i xoffset=%-4i yoffset=%-4i xadvance=%-4i page=%-4i chnl=15\n",
				ci->charcode, ci->x, ci->y, ci->width, ci->height, ci->xoffset, ci->yoffset, ci->xadvance, ci->page );
	}
	
	fclose( f );
}


//---------------------------------------------------------------------------------
//	setDrawFrames()
//---------------------------------------------------------------------------------
void FontMaker::setDrawFrames( bool state )
{
	if( state )
		_flags |= DRAW_BBOX;
	else
		_flags &= ~DRAW_BBOX;
}


//---------------------------------------------------------------------------------
//	setDrawFrames()
//---------------------------------------------------------------------------------
const char* FontMaker::fontName()
{
	if( _face )
		return _face->family_name;
	return 0;
}
