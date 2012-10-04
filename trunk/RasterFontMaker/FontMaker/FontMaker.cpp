//
//  FontMaker.cpp
//  RasterFontMaker
//
//  Created by Alexey Schutsky on 9/17/12.
//  Copyright (c) 2012 Alexey Schutsky. All rights reserved.
//

#include "FontMaker.h"

#include FT_STROKER_H

#include "xmlparser.h"

#include <assert.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

FontMaker* FontMaker::_instance = 0;


#define DRAW_BODY		0x01
#define DRAW_OUTLINE	0x02
#define DRAW_BBOX		0x04

//---------------------------------------------------------------------------------
//	FontMaker()
//---------------------------------------------------------------------------------
FontMaker::FontMaker() : _library(0), _face(0), _flags(DRAW_BODY|DRAW_OUTLINE), _fontSize(0), _pageCount(0), _lineHeight(0), _outlineWidth(0)
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
	CharInfo* ci = 0;
	
	_lineHeight = 0;
	int maxYOffset = 0;
	
	// 1. load glyphs and get metrics
	for( CharSetIt it = _charSet.begin(); it != _charSet.end(); it++ )
	{
		ci = &(*it).second;
		FT_UInt glyph_index = FT_Get_Char_Index( _face, ci->charcode );
		
		if( glyph_index == 0 )
			printf( "WARNING: no glyph for charcode 0x%X\n", ci->charcode );

		FT_Load_Glyph( _face, glyph_index, FT_LOAD_DEFAULT );
		FT_Get_Glyph( _face->glyph, &ci->glyph );
		
		ci->width = _face->glyph->metrics.width / 64 + _padding * 2; //bbox.xMax - bbox.xMin;
		ci->height = _face->glyph->metrics.height / 64 + _padding * 2; //bbox.yMax - bbox.yMin;
		
		ci->xoffset = 0;

		ci->yoffset = _face->glyph->metrics.horiBearingY / 64;
		ci->xadvance = _face->glyph->metrics.horiAdvance / 64;


		if( maxYOffset < ci->yoffset )
			maxYOffset = ci->yoffset;
		
		if( _lineHeight < ci->height )
			_lineHeight = ci->height;
	}
	
	// 2.sort by height (try to optimize layout - ?)
	// TODO: sort by height (try to optimize layout)
//	sort( _charSet.begin(), _charSet.end(), CharInfo::compareByHeight );
	
	// 3. set positions - arrange by rows & colums
	// TODO: apply padding
	_pageCount = 0;
	int x = 0;
	int y = 0;
	int maxHeight = 0;
	
	
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
		
		ci->yoffset = maxYOffset - ci->yoffset;

		ci->page = _pageCount;
		ci->x = x;
		ci->y = y;
		
		x += ci->width;
		
		if( maxHeight < ci->height )
			maxHeight = ci->height;
		
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
void FontMaker::drawPage( int page, int* abgr )
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

		FT_Glyph glyph;
		int* dstPtr = 0;
		

		if( _flags & DRAW_OUTLINE )
		{
			// copy glyph so we can use original one again after rendering
			FT_Glyph_Copy( ci->glyph, &glyph );
			
			if( glyph->format == FT_GLYPH_FORMAT_OUTLINE )
			{
				// Set up a stroker.
				FT_Stroker stroker;
				FT_Stroker_New( _library, &stroker );
				FT_Stroker_Set( stroker,
							   (int)( _outlineWidth * 64 ),
							   FT_STROKER_LINECAP_ROUND,
							   FT_STROKER_LINEJOIN_ROUND,
							   0 );
				
				FT_Glyph_StrokeBorder( &glyph, stroker, 0, 1 );

				// Render the outline spans to the span list
				SpanFuncParams user = { 0 };
				user.maker = this;
				user.buf = (PixelData32*)&abgr[ ci->x + _padding + ( ci->y + _padding ) * _imageWidth ];
				user.yOffset = _imageWidth;
				user.color = _outlineColor;
				user.outline = true;

				FT_Outline *outline = &reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;
				FT_Raster_Params params;
				memset( &params, 0, sizeof( params ) );
				params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
				params.gray_spans = spanFunc;
				params.user = &user;
				
				FT_Outline_Render( _library, outline, &params );
				
				FT_Stroker_Done( stroker );
			}

			
			FT_Done_Glyph( glyph );
		}

		if( _flags & DRAW_BODY )
		{
			// copy glyph so we can use original one again after rendering
			FT_Glyph_Copy( ci->glyph, &glyph );
			
			// Render basic glyph
			SpanFuncParams user = { 0 };
			user.maker = this;
			user.buf = (PixelData32*)&abgr[ ci->x + _padding + ( ci->y + _padding ) * _imageWidth ];
			user.yOffset = _imageWidth;
			user.color = _fontColor;
			user.outline = false;
			
			FT_Outline *outline = &reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;
			FT_Raster_Params params;
			memset( &params, 0, sizeof( params ) );
			params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
			params.gray_spans = spanFunc;
			params.user = &user;
			
			FT_Outline_Render( _library, outline, &params );

			
//			if( glyph->format != FT_GLYPH_FORMAT_BITMAP )
//			{
//				FT_Glyph_To_Bitmap( &glyph, FT_RENDER_MODE_NORMAL, 0, 1 );
//			}
//			
//			FT_BitmapGlyph glyph_bitmap = (FT_BitmapGlyph)glyph;
//			
//			dstPtr = &abgr[ ci->x + _padding + ( ci->y + _padding ) * _imageWidth ];
//			
//			
//			// copy raster data
//			FT_Bitmap bitmap = glyph_bitmap->bitmap;
//			
//			for( int y = 0; y < bitmap.rows; y++ )
//			{
//				int n = ci->x + _padding + ( ci->y + _padding + y ) * _imageWidth;
//				
//				dstPtr = &abgr[ n ];
//				for( int x = 0; x < bitmap.width; x++ )
//				{
//					if( bitmap.buffer [ x + y * bitmap.width ] != 0 )
//					{
//						*dstPtr = bitmap.buffer [ x + y * bitmap.width ] << 24;
//						
//						*dstPtr |= ( 0x00FFFFFF & color );
//					}
//					++dstPtr;
//				}
//			}
			
			FT_Done_Glyph( glyph );
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
//	spanFunc
//------------------------------------------------------------------------------
void FontMaker::spanFunc( int y, int count, const FT_Span* spans, void* user )
{
	SpanFuncParams* params = (SpanFuncParams*)user;
	PixelData32* ptr = params->buf + y * params->yOffset;
	
	if( y < params->maker->_padding * -1 )
	{
		printf( "WARNING: padding too small for outline (%i)\n", y * -1 );
		return;
	}
	
	for( int i = 0; i < count; i++ )
	{
		ptr = params->buf + y * params->yOffset + spans[i].x;
		for( int x = 0; x < spans[i].len; x++ )
		{
			if( params->outline )
			{
				ptr->r = params->color.r;
				ptr->g = params->color.g;
				ptr->b = params->color.b;
				ptr->a = spans[i].coverage;// << 24;
			}
			else
			{
				float fg = spans[i].coverage / 255.0f;
				float bg = 1 - fg;
				ptr->r = ptr->r * bg + params->color.r * fg;
				ptr->g = ptr->g * bg + params->color.g * fg;
				ptr->b = ptr->b * bg + params->color.b * fg;
			}
			
//			*ptr |= params->color; //( 0x00FF0000 & color );
			++ptr;
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

	fprintf( f, "info face=\"%s\" size=%i bold=0 italic=0 charset=\"\" unicode=1 stretchH=100 smooth=1 aa=3 padding=%i,%i,%i,%i spacing=0,0 outline=0\n",
			_face->family_name, _fontSize, _padding, _padding, _padding, _padding );
	
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
//	setDrawOutline()
//---------------------------------------------------------------------------------
void FontMaker::setDrawOutline( bool state )
{
	if( state )
		_flags |= DRAW_OUTLINE;
	else
		_flags &= ~DRAW_OUTLINE;
}


//---------------------------------------------------------------------------------
//	fontName()
//---------------------------------------------------------------------------------
const char* FontMaker::fontName()
{
	if( _face )
		return _face->family_name;
	return 0;
}


//---------------------------------------------------------------------------------
//	setFontColor()
//---------------------------------------------------------------------------------
void FontMaker::setFontColor( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	_fontColor.r = r;
	_fontColor.g = g;
	_fontColor.b = b;
	_fontColor.a = a;
}


//---------------------------------------------------------------------------------
//	setOutlineColor()
//---------------------------------------------------------------------------------
void FontMaker::setOutlineColor( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	_outlineColor.r = r;
	_outlineColor.g = g;
	_outlineColor.b = b;
	_outlineColor.a = a;
}



//---------------------------------------------------------------------------------
//	strokeChars()
//---------------------------------------------------------------------------------
bool FontMaker::strokeChars()
{
	bool result = true;
	// stroke chars and get some metrics
	
	FT_Glyph glyph;
	CharInfo* ci = 0;
	_lineHeight = 0;
	int maxYOffset = 0;

	
	// Set up a stroker.
	FT_Stroker stroker;
	FT_Stroker_New( _library, &stroker );
	FT_Stroker_Set( stroker,
				   (int)( _outlineWidth * 64 ),
				   FT_STROKER_LINECAP_ROUND,
				   FT_STROKER_LINEJOIN_ROUND,
				   0 );
	
	for( CharSetIt it = _charSet.begin(); it != _charSet.end(); it++ )
	{
		ci = &(*it).second;
		
		ci->bodySpans.clear();
		ci->outlineSpans.clear();
		
		
		FT_UInt glyph_index = FT_Get_Char_Index( _face, ci->charcode );
		
		if( glyph_index == 0 )
		{
			result = false;
			printf( "WARNING: no glyph for charcode 0x%X\n", ci->charcode );
		}
		
		FT_Load_Glyph( _face, glyph_index, FT_LOAD_DEFAULT );
		FT_Get_Glyph( _face->glyph, &ci->glyph );
		
		ci->width = _face->glyph->metrics.width / 64 + _padding * 2;
		ci->height = _face->glyph->metrics.height / 64 + _padding * 2;

		ci->xoffset = _face->glyph->metrics.horiBearingX / 64;
		ci->yoffset = _face->glyph->metrics.horiBearingY / 64;
		
		ci->xadvance = _face->glyph->metrics.horiAdvance / 64;

		
		if( _flags & DRAW_OUTLINE )
		{
			FT_Glyph_Copy( ci->glyph, &glyph );
			
			if( glyph->format == FT_GLYPH_FORMAT_OUTLINE )
			{
				
				FT_Glyph_StrokeBorder( &glyph, stroker, 0, 1 );
				
				// Render the outline spans to the span list
				
				FT_Outline *outline = &reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;
				FT_Raster_Params params;
				memset( &params, 0, sizeof( params ) );
				params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
				params.gray_spans = renderCallback;
				params.user = &ci->outlineSpans;
				
				FT_Outline_Render( _library, outline, &params );
			}
			
			
			FT_Done_Glyph( glyph );
		}
		
		if( _flags & DRAW_BODY )
		{
			FT_Glyph_Copy( ci->glyph, &glyph );
			
			FT_Outline *outline = &reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;
			FT_Raster_Params params;
			memset( &params, 0, sizeof( params ) );
			params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
			params.gray_spans = renderCallback;
			params.user = &ci->bodySpans;
			
			FT_Outline_Render( _library, outline, &params );
			
			
			FT_Done_Glyph( glyph );
		}
		
		ci->updateSize( _padding );
		
		if( _lineHeight < ci->height )
			_lineHeight = ci->height;

		if( maxYOffset < ci->yoffset )
			maxYOffset = ci->yoffset;
	}
	
	// update yoffset for all
	for( CharSetIt it = _charSet.begin(); it != _charSet.end(); it++ )
	{
		it->second.yoffset = maxYOffset - it->second.yoffset;
	}

	FT_Stroker_Done( stroker );
	
	return result;
}


//---------------------------------------------------------------------------------
//	layoutChars()
//---------------------------------------------------------------------------------
int FontMaker::layoutChars()
{
	CharInfo* ci = 0;
	_pageCount = 0;
	int x = 0;
	int y = 0;
	int maxHeight = 0;

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
		
//		ci->yoffset = maxYOffset - ci->yoffset;
		
		ci->page = _pageCount;
		ci->x = x;
		ci->y = y;
		
		x += ci->width;
		
		if( maxHeight < ci->height )
			maxHeight = ci->height;
		
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
	
	return _pageCount;}


//---------------------------------------------------------------------------------
//	drawChars()
//---------------------------------------------------------------------------------
void FontMaker::drawChars( int page, PixelData32* buf )
{
	CharInfo* ci = 0;
	
	memset( buf, 0, _imageHeight * _imageWidth * sizeof( PixelData32 ) );
	
	for( CharSetIt it = _charSet.begin(); it != _charSet.end(); it++ )
	{
		ci = &(*it).second;
		
		if( ci->page != page )
			continue;

		size_t pixIndex = 0;

		if( _flags & DRAW_OUTLINE )
		{
			// Loop over the outline spans and just draw them into the image.
			for( Spans::iterator it = ci->outlineSpans.begin(); it != ci->outlineSpans.end(); ++it )
			{
				pixIndex = ci->x - ci->xMin + it->x + _padding + ( ci->y + ci->height + ci->yMin - it->y - _padding ) * _imageWidth;
				for( int w = 0; w < it->width; ++w )
				{
					buf[pixIndex].r = _outlineColor.r;
					buf[pixIndex].g = _outlineColor.g;
					buf[pixIndex].b = _outlineColor.b;
					buf[pixIndex].a = it->coverage;
					++pixIndex;
				}
			}
		}

		// Then loop over the regular glyph spans and blend them into the image.
		for( Spans::iterator it = ci->bodySpans.begin(); it != ci->bodySpans.end(); ++it )
		{
			pixIndex = ci->x - ci->xMin + it->x + _padding + ( ci->y + ci->height + ci->yMin - it->y - _padding ) * _imageWidth;
			for( int w = 0; w < it->width; ++w )
			{
				if( _flags & DRAW_OUTLINE )
				{
					float fg = it->coverage / 255.0f;
					float bg = 1 - fg;
					buf[pixIndex].r = _outlineColor.r * bg + _fontColor.r * fg;
					buf[pixIndex].g = _outlineColor.g * bg + _fontColor.g * fg;
					buf[pixIndex].b = _outlineColor.b * bg + _fontColor.b * fg;
				}
				else
				{
					buf[pixIndex].r = _fontColor.r;
					buf[pixIndex].g = _fontColor.g;
					buf[pixIndex].b = _fontColor.b;
					buf[pixIndex].a = it->coverage;
				}
				++pixIndex;
			}
		}

		if( _flags & DRAW_BBOX )
		{
			// top line
			pixIndex = ci->x + ci->y * _imageWidth;
			for( int x = 0; x < ci->width; x++ )
			{
				buf[pixIndex].r = 0xFF;
				buf[pixIndex].a = 0xFF;
				++pixIndex;
			}
			// bottom line
			pixIndex = ci->x + ( ci->y + ci->height ) * _imageWidth;
			for( int x = 0; x < ci->width; x++ )
			{
				buf[pixIndex].r = 0xFF;
				buf[pixIndex].a = 0xFF;
				++pixIndex;
			}
			// left line
			pixIndex = ci->x + ci->y * _imageWidth;
			for( int y = 0; y < ci->height; y++ )
			{
				buf[pixIndex].r = 0xFF;
				buf[pixIndex].a = 0xFF;
				pixIndex += _imageWidth;
			}
			// right line
			pixIndex = ci->x + ci->width + ci->y * _imageWidth;
			for( int y = 0; y < ci->height; y++ )
			{
				buf[pixIndex].r = 0xFF;
				buf[pixIndex].a = 0xFF;
				pixIndex += _imageWidth;
			}
			
		}
	}
}


//---------------------------------------------------------------------------------
//	renderCallback()
//---------------------------------------------------------------------------------
void FontMaker::renderCallback( int y, int count, const FT_Span* spans, void* user )
{
	Spans *sptr = (Spans *)user;
	for( int i = 0; i < count; ++i )
		sptr->push_back( Span( spans[i].x, y, spans[i].len, spans[i].coverage ) );
}

