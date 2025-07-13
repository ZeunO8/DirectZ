@interface DZWindow : 
{
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

    window_ptr->drag_in_progress = false;
    window_ptr->event_interface->cursor_press(0, false);

    if (self.eventInterface)
        [self.eventInterface cursor_press:0 pressed:NO];
}

@end