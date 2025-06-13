@interface WINDOWDelegate : NSObject <NSWindowDelegate>
{
    dz::WINDOW* window_ptr;
}
- (instancetype)initWithWindow:(WINDOW*)window;
@end