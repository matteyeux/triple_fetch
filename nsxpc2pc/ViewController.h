#import <UIKit/UIKit.h>

@interface ViewController : UIViewController<UIPickerViewDataSource, UIPickerViewDelegate>
@property (weak, nonatomic) IBOutlet UITextView *statusTextView;
//@property (weak, nonatomic) IBOutlet UIButton *psButton;
@property (weak, nonatomic) IBOutlet UIPickerView *picker;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *getPSButton;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *execButton;

- (IBAction)getPSButtonClicked:(UIBarButtonItem *)sender;
- (IBAction)execButtonClicked:(id)sender;

- (void)logMsg:(NSString*)msg;
- (IBAction)psButtonClicked:(id)sender;


@end

