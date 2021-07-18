#include "stupisaw.h"
#include <iostream>
#include <sstream>
#include <CoreFoundation/CoreFoundation.h>
#include <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>

#if USE_MAC_UI
@interface StupiSawNSView : NSView{
    BaconPaul::StupiSawMacUI *editor;
}
    - (id)initWithEditor:(BaconPaul::StupiSawMacUI *)cont frame:(NSRect)rect;
    - (void)drawRect:(NSRect)dirtyRect;
    - (void)layout;
@end


namespace BaconPaul
{
#if USE_MAC_UI

struct StupiSawMacUI
{
    StupiSawMacUI(StupiSaw *p) : processor(p){
        CFTimeInterval TIMER_INTERVAL = .05;
        CFRunLoopTimerContext TimerContext = {0, this, nullptr, nullptr, nullptr};
        CFAbsoluteTime FireTime = CFAbsoluteTimeGetCurrent() + TIMER_INTERVAL;
        idleTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, FireTime, TIMER_INTERVAL, 0, 0,
                                         timerCallback, &TimerContext);
        if (idleTimer)
            CFRunLoopAddTimer(CFRunLoopGetMain(), idleTimer, kCFRunLoopCommonModes);
    }
    ~StupiSawMacUI()
    {
        if (idleTimer)
        {
            CFRunLoopTimerInvalidate(idleTimer);
        }
    }
    void idle()
    {
        StupiSaw::ToUI r;
        while (processor->toUiQ.try_dequeue(r))
        {
            switch(r.type)
            {
            case StupiSaw::ToUI::MIDI_NOTE_ON:
                noteOnCount++;
                [v setNeedsDisplay:TRUE];
                break;

            case StupiSaw::ToUI::MIDI_NOTE_OFF:
                noteOnCount--;
                [v setNeedsDisplay:TRUE];
                break;

            case StupiSaw::ToUI::PARAM_VALUE:

                [v setNeedsDisplay:TRUE];
                break;

            }
        }
    }
    static void timerCallback(CFRunLoopTimerRef timer, void *info)
    {
        auto that = static_cast<StupiSawMacUI*>(info);
        that->idle();
    }
    StupiSawNSView *v;
    StupiSaw *processor;
    CFRunLoopTimerRef  idleTimer;
    int noteOnCount{0};
};


bool StupiSaw::guiCocoaAttach(void *nsView) noexcept {
    NSView *view = (NSView *)nsView;

    std::cout << "Creating mac UI " << ( [view isHidden] ? "HIDDEN" : "VISIBLE" )<< std::endl;
    editor = new StupiSawMacUI(this); // how to delete?

    [view setNeedsLayout: true];
    [view setFrameSize: NSMakeSize(guiw,guih)];
    auto v = [[StupiSawNSView alloc] initWithEditor: editor frame:[view frame]];

    [view addSubview: [v retain]];
    editor->v = [v retain];

    [v release];

    NSLog(@"view frame : %.2f, %.2f", view.frame.size.width, view.frame.size.height);
    return true;
}
#endif
}


@implementation StupiSawNSView
    - (void)drawRect:(NSRect)dirtyRect
    {
        auto context = [[NSGraphicsContext currentContext] CGContext];

        CGContextSetRGBStrokeColor(context, 0, 1, 0, 1);
        CGContextSetLineWidth(context, 10.0);

        CGContextMoveToPoint(context, 0, 0);
        CGContextAddLineToPoint(context, 200, 100 + rand() % 100);
        CGContextStrokePath(context);

        CGRect textRect = CGRectMake(10,280,200,20);
        NSMutableParagraphStyle* textStyle = NSMutableParagraphStyle.defaultParagraphStyle.mutableCopy;
        textStyle.alignment = NSTextAlignmentLeft;

        NSDictionary* textFontAttributes = @{NSFontAttributeName:
                                             [NSFont fontWithName: @"Helvetica" size: 18],
                                             NSForegroundColorAttributeName: NSColor.blackColor,
                                             NSParagraphStyleAttributeName: textStyle};

        std::ostringstream oss;
        oss << "Notes On : " << editor->noteOnCount;
        auto nom = [NSString stringWithCString:oss.str().c_str() encoding:[NSString defaultCStringEncoding]];

        [nom drawInRect: textRect withAttributes: textFontAttributes];
    }
    - (void)layout
    {
        std::cout << "Layout called" << std::endl;
        [super layout];
        auto f = [self frame];
        NSLog(@"layout frame : %.2f, %.2f", f.size.width, f.size.height);
    }

    - (id)initWithEditor:(BaconPaul::StupiSawMacUI*)cont frame:(NSRect)rect
    {
        self = [super initWithFrame:rect];
        editor = cont;
        return self;
    }
@end
#endif