//
//  AppDelegate.h
//  RasterFontMaker
//
//  Created by Alexey Schutsky on 9/17/12.
//  Copyright (c) 2012 Alexey Schutsky. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
{
	NSString* _fontPath;
	NSString* _savePath;
}

@property (assign) IBOutlet NSWindow* window;
@property (assign) IBOutlet NSImageView* imageView;
@property (assign) IBOutlet NSTextView* customSet;
@property (assign) IBOutlet NSButton* drawFrame;
@property (assign) IBOutlet NSTextField* padding;
@property (assign) IBOutlet NSTextField* fontSize;
@property (assign) IBOutlet NSTextField* fontName;
@property (assign) IBOutlet NSButton* drawOutline;
@property (assign) IBOutlet NSTextField* outlineWidth;
@property (assign) IBOutlet NSColorWell* mainColor;
@property (assign) IBOutlet NSColorWell* outlineColor;
@property (assign) IBOutlet NSButton* charsetCustom;


- (IBAction)updateImage:(id)sender;
- (IBAction)exportFont:(id)sender;
- (IBAction)changeCharset:(id)sender;
- (IBAction)setTextureWidth:(id)sender;
- (IBAction)setTextureHeight:(id)sender;
- (IBAction)chooseLoadFont:(id)sender;
- (IBAction)customSetChanged:(id)sender;
- (IBAction)fetchCharSet:(id)sender;

- (void)saveSettings;
- (void)loadSettings;
- (void)applySettings;
- (void)exportFontWithName:(NSString*)fileName andPath:(NSString*)path;

@end
