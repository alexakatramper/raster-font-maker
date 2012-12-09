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
	NSString* _projectPath;
	NSString* _charsetPath;
	int _currentPage;
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
@property (assign) IBOutlet NSButton* charset0020;
@property (assign) IBOutlet NSButton* charset00A0;
@property (assign) IBOutlet NSButton* charset0400;
@property (assign) IBOutlet NSButton* charset3000;
@property (assign) IBOutlet NSButton* charset4E00;
@property (assign) IBOutlet NSButton* charsetCustom;
@property (assign) IBOutlet NSMatrix* fontFormat;
@property (assign) IBOutlet NSPopUpButton*	textureWidth;
@property (assign) IBOutlet NSPopUpButton*	textureHeight;
@property (assign) IBOutlet NSPopUpButton*	fontStyle;
@property (assign) IBOutlet NSTextField* pageOfPages;
@property (assign) IBOutlet NSButton* showNextPage;
@property (assign) IBOutlet NSButton* showLastPage;
@property (assign) IBOutlet NSButton* ignoreMissing;


- (IBAction)updateImage:(id)sender;
- (IBAction)exportFont:(id)sender;
//- (IBAction)setTextureWidth:(id)sender;
//- (IBAction)setTextureHeight:(id)sender;
- (IBAction)chooseLoadFont:(id)sender;
- (IBAction)chooseLoadFontWithStyle:(id)sender;
- (IBAction)fetchCharSet:(id)sender;

- (IBAction)loadSettings:(id)sender;
- (IBAction)saveSettings:(id)sender;

- (IBAction)nextPage:(id)sender;
- (IBAction)lastPage:(id)sender;


- (void)saveLastSettings;
- (void)loadLastSettings;
- (void)applySettings;
- (void)applyCharSets;
- (void)exportFontWithName:(NSString*)fileName andPath:(NSString*)path;

- (void)updateFontStyles;

- (void)previewPage:(int)page;

@end
