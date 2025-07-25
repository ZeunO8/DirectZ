#if defined(MACOS)
#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#define MVIEW NSView
#elif defined(IOS)
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#define MVIEW UIView
#endif

#import <QuartzCore/CAMetalLayer.h>
#import <TargetConditionals.h>
#import <Metal/Metal.h>

@interface MetalView : MVIEW
{
@public
    dz::WINDOW* window_ptr;
}
@property (nonatomic, strong) CAMetalLayer *metalLayer;
@property (nonatomic, strong) id<MTLDevice> device;
@end

@implementation MetalView

- (instancetype)initWithFrame:
#if defined(MACOS)
    NSRect
#elif defined(IOS)
    CGRect
#endif
    frameRect
{
    self = [super initWithFrame:frameRect];
    if (self)
    {
#if defined(MACOS)
        self.wantsLayer = YES;
#endif
        CAMetalLayer *layer = [CAMetalLayer layer];
        self.layer = layer;
        self.metalLayer = layer;

        self.device = MTLCreateSystemDefaultDevice();
        self.metalLayer.device = self.device;
        self.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;

#if defined(IOS)
        self.contentScaleFactor = [UIScreen mainScreen].scale;
        self.metalLayer.contentsScale = self.contentScaleFactor;
#endif
    }
    return self;
}

+ (Class)layerClass
{
    return [CAMetalLayer class];
}

@end