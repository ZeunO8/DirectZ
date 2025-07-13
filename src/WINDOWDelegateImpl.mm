

@interface WINDOWDelegate : NSObject <NSWindowDelegate>
{
    dz::WINDOW* window_ptr;
}
- (instancetype)initWithWindow:(WINDOW*)window;
@end
@implementation WINDOWDelegate
- (instancetype)initWithWindow:(WINDOW*)window
{
    self = [super init];
    if (self)
    {
        window_ptr = window;
    }
    return self;
}
- (void)windowDidBecomeKey:(NSNotification *)notification
{
	auto& window = *window_ptr;
	window_set_focused(&window, true);
	CGAssociateMouseAndMouseCursorPosition(YES);
}
- (void)windowDidResignKey:(NSNotification *)notification
{
	auto& window = *window_ptr;
	window_set_focused(&window, false);
	CGAssociateMouseAndMouseCursorPosition(NO);
}
- (BOOL)windowShouldClose:(id)sender
{
	window_ptr->closed = true;
	[NSApp terminate:nil];
	return YES;
}
@end