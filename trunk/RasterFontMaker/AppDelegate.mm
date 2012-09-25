//
//  AppDelegate.m
//  RasterFontMaker
//
//  Created by Alexey Schutsky on 9/17/12.
//  Copyright (c) 2012 Alexey Schutsky. All rights reserved.
//

#import "AppDelegate.h"
#include "FontMaker.h"
#include <set>


@implementation AppDelegate


bool _useCustom = false;


- (void)dealloc
{
    [super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	// Insert code here to initialize your application
	[self loadSettings];
}





- (IBAction)updateImage:(id)sender
{
	FontMaker* maker = FontMaker::instance();
	
	if( ! maker->fontLoaded() )
	{
		NSAlert* alert = [NSAlert alertWithMessageText:@"No font loaded."
										 defaultButton:nil
									   alternateButton:nil
										   otherButton:nil
							 informativeTextWithFormat:@"You have to load а font at first."];
		[alert runModal];
		return;
	}

	[self applySettings];
	
	int width = maker->imageWidth();
	int height = maker->imageHeight();
	
	if( _useCustom )
	{
		NSString* chars = [_customSet string];
		for( NSInteger i = 0; i < [chars length]; i++ )
		{
			maker->addChar( [chars characterAtIndex:i] );
		}
	}
	
	// create image
	NSBitmapImageRep* bmp = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
																	pixelsWide:width
																	pixelsHigh:height
																 bitsPerSample:8
															   samplesPerPixel:4
																	  hasAlpha:YES
																	  isPlanar:NO
																colorSpaceName:NSCalibratedRGBColorSpace
																   bytesPerRow:0
																  bitsPerPixel:0];
	
	
	

	// draw to image
	
	
//	maker->setFontSize( 64 );
		
	/*int pages = */
	maker->makeLayout();

	unsigned char* data = [bmp bitmapData];
	int* rgbaData = (int*)data;
	maker->drawPage( 0, rgbaData );

	// update image view
	NSSize size;
	size.width = width;
	size.height = height;
	NSImage* image = [[NSImage alloc] initWithCGImage:[bmp CGImage] size:size];
	
	[_imageView setImage:image];

}


- (IBAction)exportFont:(id)sender
{
	if( ! FontMaker::instance()->fontLoaded() )
	{
		NSAlert* alert = [NSAlert alertWithMessageText:@"No font loaded."
										 defaultButton:nil
									   alternateButton:nil
										   otherButton:nil
							 informativeTextWithFormat:@"You have to load а font at first."];
		[alert runModal];
		return;
	}

	NSSavePanel* panel = [NSSavePanel savePanel];
	if( _savePath )
		[panel setDirectoryURL:[NSURL fileURLWithPath:_savePath isDirectory:YES]];
	
	
	
	[panel setNameFieldStringValue:[NSString stringWithFormat:@"%s_%li.fnt", FontMaker::instance()->fontName(), [_fontSize integerValue] ]];
													//FontMaker::instance()->fontSize() ]];
//	[panel setExtensionHidden:NO];
	[panel setCanSelectHiddenExtension:YES];
//	[panel setCanChooseDirectories:NO];
//	[panel setCanChooseFiles:YES];
//	[panel setAllowsMultipleSelection:NO];
	[panel setAllowedFileTypes:[NSArray arrayWithObject:@"fnt"]];
	
	[panel beginSheetModalForWindow:_window completionHandler:^(NSInteger result)
	 {
		 if( result == NSFileHandlingPanelOKButton )
		 {
			 NSString* fileName = [[panel.URL URLByDeletingPathExtension] lastPathComponent];
			 
			 if( _savePath )
				 [_savePath release];
			 _savePath = [[panel.directoryURL path] retain];
			 [self exportFontWithName:fileName andPath:_savePath];
			 [self saveSettings];
		 }
	 }];
}

- (void)exportFontWithName:(NSString*)fileName andPath:(NSString*)path
{
	FontMaker* maker = FontMaker::instance();

	[self applySettings];
	[self saveSettings];

	int width = maker->imageWidth();
	int height = maker->imageHeight();
	
	if( _useCustom )
	{
		NSString* chars = [_customSet string];
		for( NSInteger i = 0; i < [chars length]; i++ )
		{
			maker->addChar( [chars characterAtIndex:i] );
		}
	}

	int pages = maker->makeLayout();
	
//	NSString* imageFileName = [NSString stringWithCString:"/Users/Schutsky/Desktop/testfont2" encoding:NSASCIIStringEncoding ];
	
	for( int i = 0; i < pages; i++ )
	{
		// create image
		NSBitmapImageRep* bmp = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
																		pixelsWide:width
																		pixelsHigh:height
																	 bitsPerSample:8
																   samplesPerPixel:4
																		  hasAlpha:YES
																		  isPlanar:NO
																	colorSpaceName:NSCalibratedRGBColorSpace
																	   bytesPerRow:0
																	  bitsPerPixel:0];
	
		unsigned char* data = [bmp bitmapData];
		int* rgbaData = (int*)data;
		maker->drawPage( i, rgbaData );
		
		CFURLRef url = 0;

		if( pages == 1 )
			url = (CFURLRef)[NSURL fileURLWithPath:[path stringByAppendingFormat:@"/%@.png", fileName ]];
		else
			url = (CFURLRef)[NSURL fileURLWithPath:[path stringByAppendingFormat:@"/%@_%i.png", fileName, i ]];
			
		CGImageDestinationRef destination = CGImageDestinationCreateWithURL( url, kUTTypePNG, 1, NULL );
		CGImageDestinationAddImage( destination, [bmp CGImage], nil );
		
		if( !CGImageDestinationFinalize( destination ) )
		{
			NSLog( @"Failed to write image to %@", url );
		}
		
		CFRelease( destination );
	}
//	maker->exportTXT( "/Users/Schutsky/Desktop/testfont" );
	maker->exportXML( [fileName cStringUsingEncoding:NSASCIIStringEncoding], [path cStringUsingEncoding:NSASCIIStringEncoding] );
}


- (IBAction)changeCharset:(id)sender
{
	if( [[sender identifier] compare:@"0x0020"] == NSOrderedSame )
	{
		if( [(NSButton*)sender state] == NSOnState )
			FontMaker::instance()->addCharRange( 0x0020, 0x007F);
		else
			FontMaker::instance()->removeCharRange( 0x0020, 0x007F);
		return;
	}

	if( [[sender identifier] compare:@"0x00A0"] == NSOrderedSame )
	{
		if( [(NSButton*)sender state] == NSOnState )
			FontMaker::instance()->addCharRange( 0x00A0, 0x00FF);
		else
			FontMaker::instance()->removeCharRange( 0x00A0, 0x00FF);
		return;
	}

	if( [[sender identifier] compare:@"0x0400"] == NSOrderedSame )
	{
		if( [(NSButton*)sender state] == NSOnState )
			FontMaker::instance()->addCharRange( 0x0400, 0x04FF);
		else
			FontMaker::instance()->removeCharRange( 0x0400, 0x04FF);
		return;
	}

	if( [[sender identifier] compare:@"0x3000"] == NSOrderedSame )
	{
		if( [(NSButton*)sender state] == NSOnState )
			FontMaker::instance()->addCharRange( 0x3000, 0x30FF);
		else
			FontMaker::instance()->removeCharRange( 0x3000, 0x30FF);
		return;
	}
	
	if( [[sender identifier] compare:@"0x4E00"] == NSOrderedSame )
	{
		if( [(NSButton*)sender state] == NSOnState )
			FontMaker::instance()->addCharRange( 0x4E00, 0x9FFF);
		else
			FontMaker::instance()->removeCharRange( 0x4E00, 0x9FFF);
		return;
	}
	
	if( [[sender identifier] compare:@"0xXXXX"] == NSOrderedSame )
	{
		if( [(NSButton*)sender state] == NSOnState )
			_useCustom = true;
		else
			_useCustom = false;
		return;
	}
	
}


- (IBAction)setTextureWidth:(id)sender
{
	NSInteger i = [(NSPopUpButton*)sender indexOfSelectedItem];
	switch( i )
	{
		case 0:
			FontMaker::instance()->setImageWidth( 64 );
			break;
		case 1:
			FontMaker::instance()->setImageWidth( 128 );
			break;
		case 2:
			FontMaker::instance()->setImageWidth( 256 );
			break;
		case 3:
			FontMaker::instance()->setImageWidth( 512 );
			break;
		case 4:
			FontMaker::instance()->setImageWidth( 1024 );
			break;
		case 5:
		default:
			FontMaker::instance()->setImageWidth( 2048 );
			break;
	}
}


- (IBAction)setTextureHeight:(id)sender
{
	NSInteger i = [(NSPopUpButton*)sender indexOfSelectedItem];
	switch( i )
	{
		case 0:
			FontMaker::instance()->setImageHeight( 64 );
			break;
		case 1:
			FontMaker::instance()->setImageHeight( 128 );
			break;
		case 2:
			FontMaker::instance()->setImageHeight( 256 );
			break;
		case 3:
			FontMaker::instance()->setImageHeight( 512 );
			break;
		case 4:
			FontMaker::instance()->setImageHeight( 1024 );
			break;
		case 5:
		default:
			FontMaker::instance()->setImageHeight( 2048 );
			break;
	}
}


- (IBAction)chooseLoadFont:(id)sender
{
	NSOpenPanel* panel = [NSOpenPanel openPanel];
	[panel setCanChooseDirectories:NO];
	[panel setCanChooseFiles:YES];
	[panel setAllowsMultipleSelection:NO];
	[panel setAllowedFileTypes:[NSArray arrayWithObject:@"ttf"]];
	if( _fontPath )
		[panel setDirectoryURL:[NSURL fileURLWithPath:_fontPath isDirectory:NO]];

	
	[panel beginSheetModalForWindow:_window completionHandler:^(NSInteger result)
	{
		if( result == NSFileHandlingPanelOKButton )
		{
			[_fontPath release];
			_fontPath = [[panel.URL path] retain];
			
			FontMaker::instance()->loadFont( [_fontPath cStringUsingEncoding:NSASCIIStringEncoding] );
			
			[_fontName setStringValue:[NSString stringWithCString:FontMaker::instance()->fontName() encoding:NSASCIIStringEncoding]];
			
			[self saveSettings];
		}
	}];
}


- (IBAction)customSetChanged:(id)sender
{
	
}


- (void)saveSettings
{
//
	CFPreferencesSetAppValue( CFSTR("SavePath"), _savePath,  kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("FontPath"), _fontPath,  kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("Padding"), [_padding stringValue], kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("Frames"), ( ( [_drawFrame state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("CustomCharset"), [_customSet string], kCFPreferencesCurrentApplication );

	CFPreferencesSetAppValue( CFSTR("FontSize"), [_fontSize stringValue], kCFPreferencesCurrentApplication );
//	CFPreferencesSetAppValue( CFSTR("FontColor"), [_mainColor color], kCFPreferencesCurrentApplication );

	CFPreferencesSetAppValue( CFSTR("DrawOutline"), ( ( [_drawOutline state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("OutlineWidth"), [_outlineWidth stringValue], kCFPreferencesCurrentApplication );
//	CFPreferencesSetAppValue( CFSTR("OutlineColor"), [_outlineColor color], kCFPreferencesCurrentApplication );
	
//	CFPreferencesSetAppValue( CFSTR("Charset0000"), ( ( [_drawOutline state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	
	CFPreferencesSetAppValue( CFSTR("CharsetXXXX"), ( ( [_charsetCustom state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	
	
	CFPreferencesAppSynchronize(  kCFPreferencesCurrentApplication );
}


- (void)loadSettings
{
	_savePath = (NSString*)CFPreferencesCopyAppValue( CFSTR("SavePath"), kCFPreferencesCurrentApplication );
	_fontPath = (NSString*)CFPreferencesCopyAppValue( CFSTR("FontPath"), kCFPreferencesCurrentApplication );
	if( _fontPath )
	{
		FontMaker::instance()->loadFont( [_fontPath cStringUsingEncoding:NSASCIIStringEncoding] );
		[_fontName setStringValue:[NSString stringWithCString:FontMaker::instance()->fontName() encoding:NSASCIIStringEncoding]];
	}

	NSString* value;
	NSColor* color;
	
	
	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("Padding"), kCFPreferencesCurrentApplication );
	if( value )
		[_padding setStringValue:value];
	else
		[_padding setIntegerValue:0];

	
	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("Frames"), kCFPreferencesCurrentApplication );
	if( value && ( [value compare:@"Yes"] == NSOrderedSame ) )
		[_drawFrame setState:NSOnState];
	else
		[_drawFrame setState:NSOffState];

	
	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("CustomCharset"), kCFPreferencesCurrentApplication );
	[_customSet setString:value];
	
	
	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("FontSize"), kCFPreferencesCurrentApplication );
	if( value )
		[_fontSize setStringValue:value];
	else
		[_fontSize setIntegerValue:16];

	
	color = (NSColor*)CFPreferencesCopyAppValue( CFSTR("FontColor"), kCFPreferencesCurrentApplication );
	if( value )
		[_mainColor setColor:color];
	
	
	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("DrawOutline"), kCFPreferencesCurrentApplication );
	if( value && ( [value compare:@"Yes"] == NSOrderedSame ) )
		[_drawOutline setState:NSOnState];
	else
		[_drawOutline setState:NSOffState];
	

	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("OutlineWidth"), kCFPreferencesCurrentApplication );
	[_outlineWidth setStringValue:value];


	color = (NSColor*)CFPreferencesCopyAppValue( CFSTR("OutlineColor"), kCFPreferencesCurrentApplication );
	if( value )
		[_outlineColor setColor:color];


	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("CharsetXXXX"), kCFPreferencesCurrentApplication );
	if( value && ( [value compare:@"Yes"] == NSOrderedSame ) )
		[_charsetCustom setState:NSOnState];
	else
		[_charsetCustom setState:NSOffState];
	
}


- (void)applySettings
{
	FontMaker* maker = FontMaker::instance();
	
	if( _fontPath )
	{
		maker->loadFont( [_fontPath cStringUsingEncoding:NSASCIIStringEncoding] );
		[_fontName setStringValue:[NSString stringWithCString:FontMaker::instance()->fontName() encoding:NSASCIIStringEncoding]];
	}
	
	maker->setDrawFrames( [_drawFrame state] == NSOnState );
		
	NSInteger pad = [_padding integerValue];
	maker->setPadding( (int)pad );
	
	NSInteger fSize = [_fontSize integerValue];
	maker->setFontSize( (int)fSize );


	maker->setDrawOutline( [_drawOutline state] == NSOnState );
	maker->setOutlineWidth( [_outlineWidth floatValue] );

	
	CGFloat r = 0;
	CGFloat g = 0;
	CGFloat b = 0;
	CGFloat a = 0;
	
	NSColor* fontColor = [[_mainColor color] colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
//	NSColor* fontColor = [_mainColor color];
	[fontColor getRed:&r green:&g blue:&b alpha:&a];
	
	maker->setFontColor( (unsigned char)( r * 255 ), (unsigned char)( g * 255 ), (unsigned char)( b * 255 ), (unsigned char)( a * 255 ) );

	fontColor = [[_outlineColor color] colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
	[fontColor getRed:&r green:&g blue:&b alpha:&a];
	
	maker->setOutlineColor( (unsigned char)( r * 255 ), (unsigned char)( g * 255 ), (unsigned char)( b * 255 ), (unsigned char)( a * 255 ) );
}



- (IBAction)fetchCharSet:(id)sender
{
	NSOpenPanel* panel = [NSOpenPanel openPanel];
	[panel setCanChooseDirectories:NO];
	[panel setCanChooseFiles:YES];
	[panel setAllowsMultipleSelection:NO];
//	[panel setAllowedFileTypes:[NSArray arrayWithObject:@"ttf"]];
	[panel beginSheetModalForWindow:_window completionHandler:^(NSInteger result)
	 {
		 if( result == NSFileHandlingPanelOKButton )
		 {
			 NSError* error = 0;
			 NSString* fileContent = [NSString stringWithContentsOfFile:[panel.URL path] encoding:NSUTF8StringEncoding error:&error];
			 if( error == nil )
			 {
				 std::set<unichar> charset;
				 for( NSUInteger i = 0; i < [fileContent length]; i++ )
				 {
					 unichar uch = [fileContent characterAtIndex:i];
					 if( uch < 0x0020 )	// skip control symbols
						 continue;
					 charset.insert( uch );
				 }
				 NSMutableString* setStr = [NSMutableString stringWithCapacity:charset.size()];
				 
				 for( std::set<unichar>::iterator it = charset.begin(); it != charset.end(); it++ )
					 [setStr appendFormat:@"%C", (*it) ];
				 
				 [_customSet setString:setStr];

				 [self saveSettings];
			 }
			 
			 
		 }
	 }];
}


@end
