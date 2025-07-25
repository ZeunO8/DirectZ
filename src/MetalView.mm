#if defined(MACOS)
#include <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#include <ApplicationServices/ApplicationServices.h>
#elif defined(IOS)
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#endif
#import <QuartzCore/CAMetalLayer.h>
#import <TargetConditionals.h>
#include <Metal/Metal.h>

@interface MetalView : NSView
@property (nonatomic, strong) CAMetalLayer *metalLayer;
@property (nonatomic, strong) id<MTLDevice> device;
@end

@implementation MetalView {

}
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.wantsLayer = YES;
        self.layer = [CAMetalLayer layer];
        self.metalLayer = (CAMetalLayer*)self.layer;
        self.device = MTLCreateSystemDefaultDevice();
        self.metalLayer.device = self.device;
        self.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    }
    return self;
}

@end