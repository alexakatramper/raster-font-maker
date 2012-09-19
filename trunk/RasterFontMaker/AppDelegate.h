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
}

@property (assign) IBOutlet NSWindow* window;
@property (assign) IBOutlet NSImageView* imageView;
@property (assign) IBOutlet NSTextView* customSet;
@property (assign) IBOutlet NSButton* drawFrame;
@property (assign) IBOutlet NSTextField* padding;
@property (assign) IBOutlet NSTextField* fontSize;
@property (assign) IBOutlet NSTextField* fontName;


- (IBAction)updateImage:(id)sender;
- (IBAction)exportFont:(id)sender;
- (IBAction)changeCharset:(id)sender;
- (IBAction)setTextureWidth:(id)sender;
- (IBAction)setTextureHeight:(id)sender;
- (IBAction)chooseLoadFont:(id)sender;
- (IBAction)customSetChanged:(id)sender;

- (void)saveSettings;
- (void)loadSettings;
- (void)applySettings;

@end
