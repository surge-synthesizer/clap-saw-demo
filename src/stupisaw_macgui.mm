#include "stupisaw.h"
#include <iostream>
#include <CoreFoundation/CoreFoundation.h>
#include <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>

#if USE_MAC_UI
@interface StupiSawNSView : NSView{

}
    - (void)drawRect:(NSRect)dirtyRect;
    - (void)layout;
@end

@implementation StupiSawNSView
    - (void)drawRect:(NSRect)dirtyRect
    {
        auto context = [[NSGraphicsContext currentContext] CGContext];

        std::cout << "DrawRect called" << std::endl;
        CGContextSetRGBStrokeColor(context, 0, 1, 0, 1);
        CGContextSetLineWidth(context, 10.0);

        CGContextMoveToPoint(context, 0, 0);
        CGContextAddLineToPoint(context, 200, 100);
        CGContextStrokePath(context);
    }
    - (void)layout
    {
        std::cout << "Layout called" << std::endl;
        [super layout];
        auto f = [self frame];
        NSLog(@"layout frame : %.2f, %.2f", f.size.width, f.size.height);
    }
@end
#endif

namespace BaconPaul
{
#if USE_MAC_UI

struct StupiSawMacUI
{
    StupiSawNSView *v;
};


bool StupiSaw::guiCocoaAttach(void *nsView) noexcept {
    NSView *view = (NSView *)nsView;

    std::cout << "Creating mac UI " << ( [view isHidden] ? "HIDDEN" : "VISIBLE" )<< std::endl;
    editor = new StupiSawMacUI(); // how to delete?

    [view setNeedsLayout: true];
    [view setFrameSize: NSMakeSize(guiw,guih)];
    auto v = [[StupiSawNSView alloc] initWithFrame:[view frame]];

    [view addSubview: [v retain]];
    editor->v = [v retain];

    [v release];


    NSLog(@"view frame : %.2f, %.2f", view.frame.size.width, view.frame.size.height);
    return true;
}
#endif
}