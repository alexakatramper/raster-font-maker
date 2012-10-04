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
//	maker->makeLayout();
	
	if( ! maker->strokeChars() )
	{
		NSAlert* alert = [NSAlert alertWithMessageText:@"Missing glyph(s)."
										 defaultButton:nil
									   alternateButton:nil
										   otherButton:nil
							 informativeTextWithFormat:@"One ore more glyps are absent in the current font."];
		[alert runModal];
	}
	maker->layoutChars();
	

	unsigned char* data = [bmp bitmapData];
	int* rgbaData = (int*)data;
//	maker->drawPage( 0, rgbaData );
	maker->drawChars( 0, (PixelData32*)rgbaData );

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
	

//	int pages = maker->makeLayout();
	if( ! maker->strokeChars() )
	{
		NSAlert* alert = [NSAlert alertWithMessageText:@"Missing glyph(s)."
										 defaultButton:nil
									   alternateButton:nil
										   otherButton:nil
							 informativeTextWithFormat:@"One ore more glyps are absent in the current font."];
		[alert runModal];
	}
	int pages = maker->layoutChars();

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
//		maker->drawPage( i, rgbaData );
		maker->drawChars( i, (PixelData32*)rgbaData );

		
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
	
	if( [_fontFormat selectedRow] == 0 )
		maker->exportTXT( [fileName cStringUsingEncoding:NSASCIIStringEncoding], [path cStringUsingEncoding:NSASCIIStringEncoding] );
	else
		maker->exportXML( [fileName cStringUsingEncoding:NSASCIIStringEncoding], [path cStringUsingEncoding:NSASCIIStringEncoding] );
}


- (void)applyCharSets
{
	FontMaker* maker = FontMaker::instance();
	
	maker->resetCharSet();

//	if( [[sender identifier] compare:@"0x0020"] == NSOrderedSame )
	if( [_charset0020 state] == NSOnState )
		maker->addCharRange( 0x0020, 0x007E);	//0x007F is 'del'

	if( [_charset00A0 state] == NSOnState )
		maker->addCharRange( 0x00A0, 0x00FF);

	if( [_charset0400 state] == NSOnState )
		maker->addCharRange( 0x0400, 0x04FF);

	if( [_charset3000 state] == NSOnState )
		maker->addCharRange( 0x3000, 0x30FF);
	
	if( [_charset4E00 state] == NSOnState )
		maker->addCharRange( 0x4E00, 0x9FFF);
	
	if( [_charsetCustom state] == NSOnState )
	{
		NSString* chars = [_customSet string];
		for( NSInteger i = 0; i < [chars length]; i++ )
		{
			maker->addChar( [chars characterAtIndex:i] );
		}
	}
	
}


//- (IBAction)setTextureWidth:(id)sender
//{
//	NSInteger i = [(NSPopUpButton*)sender indexOfSelectedItem];
//	switch( i )
//	{
//		case 0:
//			FontMaker::instance()->setImageWidth( 64 );
//			break;
//		case 1:
//			FontMaker::instance()->setImageWidth( 128 );
//			break;
//		case 2:
//			FontMaker::instance()->setImageWidth( 256 );
//			break;
//		case 3:
//			FontMaker::instance()->setImageWidth( 512 );
//			break;
//		case 4:
//			FontMaker::instance()->setImageWidth( 1024 );
//			break;
//		case 5:
//		default:
//			FontMaker::instance()->setImageWidth( 2048 );
//			break;
//	}
//}
//
//
//- (IBAction)setTextureHeight:(id)sender
//{
//	NSInteger i = [(NSPopUpButton*)sender indexOfSelectedItem];
//	switch( i )
//	{
//		case 0:
//			FontMaker::instance()->setImageHeight( 64 );
//			break;
//		case 1:
//			FontMaker::instance()->setImageHeight( 128 );
//			break;
//		case 2:
//			FontMaker::instance()->setImageHeight( 256 );
//			break;
//		case 3:
//			FontMaker::instance()->setImageHeight( 512 );
//			break;
//		case 4:
//			FontMaker::instance()->setImageHeight( 1024 );
//			break;
//		case 5:
//		default:
//			FontMaker::instance()->setImageHeight( 2048 );
//			break;
//	}
//}


- (IBAction)chooseLoadFont:(id)sender
{
	NSOpenPanel* panel = [NSOpenPanel openPanel];
	[panel setCanChooseDirectories:NO];
	[panel setCanChooseFiles:YES];
	[panel setAllowsMultipleSelection:NO];
//	[panel setAllowedFileTypes:[NSArray arrayWithObject:@"ttf"]];
	[panel setAllowedFileTypes:[NSArray arrayWithObjects:@"ttf",@"otf",@"ttc",nil]];
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
	CFPreferencesSetAppValue( CFSTR("CharsetPath"), _charsetPath,  kCFPreferencesCurrentApplication );
	
	CFPreferencesSetAppValue( CFSTR("Padding"), [_padding stringValue], kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("Frames"), ( ( [_drawFrame state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("CustomCharset"), [_customSet string], kCFPreferencesCurrentApplication );

	CFPreferencesSetAppValue( CFSTR("FontSize"), [_fontSize stringValue], kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("ExportFormat"), ( [_fontFormat selectedRow] == 0 ) ? CFSTR( "TXT" ) : CFSTR( "XML" ), kCFPreferencesCurrentApplication );

	NSColor* color = [[_mainColor color] colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
	CFPreferencesSetAppValue( CFSTR("FontColor.r"), [NSString stringWithFormat:@"%f",[color redComponent]], kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("FontColor.g"), [NSString stringWithFormat:@"%f",[color greenComponent]], kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("FontColor.b"), [NSString stringWithFormat:@"%f",[color blueComponent]], kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("FontColor.a"), [NSString stringWithFormat:@"%f",[color alphaComponent]], kCFPreferencesCurrentApplication );

	
	CFPreferencesSetAppValue( CFSTR("DrawOutline"), ( ( [_drawOutline state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("OutlineWidth"), [_outlineWidth stringValue], kCFPreferencesCurrentApplication );
	
	color = [[_outlineColor color] colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
	CFPreferencesSetAppValue( CFSTR("OutlineColor.r"), [NSString stringWithFormat:@"%f",[color redComponent]], kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("OutlineColor.g"), [NSString stringWithFormat:@"%f",[color greenComponent]], kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("OutlineColor.b"), [NSString stringWithFormat:@"%f",[color blueComponent]], kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("OutlineColor.a"), [NSString stringWithFormat:@"%f",[color alphaComponent]], kCFPreferencesCurrentApplication );
	
	CFPreferencesSetAppValue( CFSTR("Charset0020"), ( ( [_charset0020 state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("Charset00A0"), ( ( [_charset00A0 state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("Charset0400"), ( ( [_charset0400 state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("Charset3000"), ( ( [_charset3000 state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("Charset4E00"), ( ( [_charset4E00 state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("CharsetXXXX"), ( ( [_charsetCustom state] == NSOnState ) ? CFSTR("Yes") : CFSTR("No") ), kCFPreferencesCurrentApplication );
	
	CFPreferencesSetAppValue( CFSTR("TextureWidth"), [_textureWidth titleOfSelectedItem], kCFPreferencesCurrentApplication );
	CFPreferencesSetAppValue( CFSTR("TextureHeight"), [_textureHeight titleOfSelectedItem], kCFPreferencesCurrentApplication );

	
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

	_charsetPath = (NSString*)CFPreferencesCopyAppValue( CFSTR("CharsetPath"), kCFPreferencesCurrentApplication );

	
	NSString* value;	
	
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

	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("ExportFormat"), kCFPreferencesCurrentApplication );
	if( value && ( [value compare:@"XML"] == NSOrderedSame ) )
		[_fontFormat selectCellAtRow:1 column:0];
	else
		[_fontFormat selectCellAtRow:0 column:0];

	
	NSString* rColor = (NSString*)CFPreferencesCopyAppValue( CFSTR("FontColor.r"), kCFPreferencesCurrentApplication );
	NSString* gColor = (NSString*)CFPreferencesCopyAppValue( CFSTR("FontColor.g"), kCFPreferencesCurrentApplication );
	NSString* bColor = (NSString*)CFPreferencesCopyAppValue( CFSTR("FontColor.b"), kCFPreferencesCurrentApplication );
	NSString* aColor = (NSString*)CFPreferencesCopyAppValue( CFSTR("FontColor.a"), kCFPreferencesCurrentApplication );
	if( rColor != nil & gColor != nil & bColor != nil & aColor != nil )
		[_mainColor setColor:[NSColor colorWithCalibratedRed:[rColor floatValue]
													   green:[gColor floatValue]
														blue:[bColor floatValue]
													   alpha:[aColor floatValue]]];
	else
		[_mainColor setColor:[NSColor whiteColor]];
	
	
	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("DrawOutline"), kCFPreferencesCurrentApplication );
	if( value && ( [value compare:@"Yes"] == NSOrderedSame ) )
		[_drawOutline setState:NSOnState];
	else
		[_drawOutline setState:NSOffState];
	

	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("OutlineWidth"), kCFPreferencesCurrentApplication );
	[_outlineWidth setStringValue:value];


	rColor = (NSString*)CFPreferencesCopyAppValue( CFSTR("OutlineColor.r"), kCFPreferencesCurrentApplication );
	gColor = (NSString*)CFPreferencesCopyAppValue( CFSTR("OutlineColor.g"), kCFPreferencesCurrentApplication );
	bColor = (NSString*)CFPreferencesCopyAppValue( CFSTR("OutlineColor.b"), kCFPreferencesCurrentApplication );
	aColor = (NSString*)CFPreferencesCopyAppValue( CFSTR("OutlineColor.a"), kCFPreferencesCurrentApplication );
	if( rColor != nil & gColor != nil & bColor != nil & aColor != nil )
		[_outlineColor setColor:[NSColor colorWithCalibratedRed:[rColor floatValue]
													   green:[gColor floatValue]
														blue:[bColor floatValue]
													   alpha:[aColor floatValue]]];
	else
		[_outlineColor setColor:[NSColor blackColor]];

	
	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("Charset0020"), kCFPreferencesCurrentApplication );
	if( value && ( [value compare:@"Yes"] == NSOrderedSame ) )
		[_charset0020 setState:NSOnState];

	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("Charset00A0"), kCFPreferencesCurrentApplication );
	if( value && ( [value compare:@"Yes"] == NSOrderedSame ) )
		[_charset00A0 setState:NSOnState];

	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("Charset0400"), kCFPreferencesCurrentApplication );
	if( value && ( [value compare:@"Yes"] == NSOrderedSame ) )
		[_charset0400 setState:NSOnState];

	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("Charset3000"), kCFPreferencesCurrentApplication );
	if( value && ( [value compare:@"Yes"] == NSOrderedSame ) )
		[_charset3000 setState:NSOnState];

	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("Charset4E00"), kCFPreferencesCurrentApplication );
	if( value && ( [value compare:@"Yes"] == NSOrderedSame ) )
		[_charset4E00 setState:NSOnState];

	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("CharsetXXXX"), kCFPreferencesCurrentApplication );
	if( value && ( [value compare:@"Yes"] == NSOrderedSame ) )
		[_charsetCustom setState:NSOnState];

	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("TextureWidth"), kCFPreferencesCurrentApplication );
	if( value )
		[_textureWidth selectItemWithTitle:value];
	
	value = (NSString*)CFPreferencesCopyAppValue( CFSTR("TextureHeight"), kCFPreferencesCurrentApplication );
	if( value )
		[_textureHeight selectItemWithTitle:value];
}


- (void)applySettings
{
	FontMaker* maker = FontMaker::instance();
	
	if( _fontPath )
	{
		maker->loadFont( [_fontPath cStringUsingEncoding:NSASCIIStringEncoding] );
		[_fontName setStringValue:[NSString stringWithCString:FontMaker::instance()->fontName() encoding:NSASCIIStringEncoding]];
	}
	
	
	[self applyCharSets];

	
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
	
	
	NSInteger i = [_textureWidth indexOfSelectedItem];
	switch( i )
	{
		case 0:
			maker->setImageWidth( 64 );
			break;
		case 1:
			maker->setImageWidth( 128 );
			break;
		case 2:
			maker->setImageWidth( 256 );
			break;
		case 3:
			maker->setImageWidth( 512 );
			break;
		case 4:
			maker->setImageWidth( 1024 );
			break;
		case 5:
		default:
			maker->setImageWidth( 2048 );
			break;
	}

	
	i = [_textureHeight indexOfSelectedItem];
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



- (IBAction)fetchCharSet:(id)sender
{
	NSOpenPanel* panel = [NSOpenPanel openPanel];
	[panel setCanChooseDirectories:NO];
	[panel setCanChooseFiles:YES];
	[panel setAllowsMultipleSelection:NO];
	if( _charsetPath )
		[panel setDirectoryURL:[NSURL fileURLWithPath:_charsetPath isDirectory:NO]];
	
	[panel beginSheetModalForWindow:_window completionHandler:^(NSInteger result)
	 {
		 if( result == NSFileHandlingPanelOKButton )
		 {
			 [_charsetPath release];
			 _charsetPath = [[panel.URL path] retain];

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
