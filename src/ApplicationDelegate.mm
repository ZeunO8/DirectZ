@interface ApplicationDelegate : NSObject <NSApplicationDelegate>
{
    dz::WINDOW* window_ptr;
}
- (instancetype)initWithWindow:(WINDOW*)window;
@end

@implementation ApplicationDelegate
- (instancetype)initWithWindow:(WINDOW*)window
{
    self = [super init];
    if (self)
    {
        window_ptr = window;
    }
    return self;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    window_ptr->closed = true;
    return NSTerminateNow;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}
@end