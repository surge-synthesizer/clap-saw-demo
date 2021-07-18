#include "stupisaw.h"
#include <iostream>
#include <CoreFoundation/CoreFoundation.h>
#include <AppKit/AppKit.h>

#if USE_MAC_UI
@interface StupiSawNSView : NSView{

}
    - (void)drawRect:(NSRect)dirtyRect;
@end

@implementation StupiSawNSView
    - (void)drawRect:(NSRect)dirtyRect
    {
        std::cout << "drawREct called" << std::endl;
    }
@end
#endif

namespace BaconPaul
{
#if USE_MAC_UI

struct StupiSawMacUI
{

};



void StupiSaw::guiSize(uint32_t *width, uint32_t *height) noexcept
{
    *width = 500;
    *height = 400;
}

bool StupiSaw::guiCocoaAttach(void *nsView) noexcept {
    NSView *view = (NSView *)nsView;

    std::cout << "Creating mac UI " << ( [view isHidden] ? "HIDDEN" : "VISIBLE" )<< std::endl;
    editor = new StupiSawMacUI(); // how to delete?

    auto v = [[StupiSawNSView alloc] initWithFrame:[view frame]];
    [view addSubview: v];

    NSLog(@"view frame : %.2f, %.2f", view.frame.size.width, view.frame.size.height);
    return true;
}
#endif
}