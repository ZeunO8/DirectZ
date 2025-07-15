@interface DZWindow : NSWindow
{
@public
    dz::WINDOW* window_ptr;
}
@property (nonatomic, assign) BOOL dragInProgress;
@end

@implementation DZWindow

- (void)performWindowDragWithEvent:(NSEvent *)event
{
    self.dragInProgress = YES;
    [super performWindowDragWithEvent:event];
    self.dragInProgress = NO;

    if (window_ptr != nullptr)
    {
        window_ptr->drag_in_progress = false;
        if (window_ptr->event_interface)
        {
            window_ptr->event_interface->cursor_press(0, false);
        }
    }
}

@end