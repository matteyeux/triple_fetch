// elements of the UI based on the tutorial at: http://codewithchris.com/uipickerview-example/

#import "ViewController.h"
#include "log.h"
#include "sploit.h"
#include "drop_payload.h"

#include <CoreFoundation/CoreFoundation.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

static char* bundle_path() {
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
  int len = 4096;
  char* path = malloc(len);
  
  CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8*)path, len);
  
  return path;
}

NSArray* getBundlePocs() {
  DIR *dp;
  struct dirent *ep;
  
  char* in_path = NULL;
  char* bundle_root = bundle_path();
  asprintf(&in_path, "%s/pocs/", bundle_root);
  
  NSMutableArray* arr = [NSMutableArray array];
  
  dp = opendir(in_path);
  if (dp == NULL) {
    printf("unable to open pocs directory: %s\n", in_path);
    return NULL;
  }
  
  while ((ep = readdir(dp))) {
    if (ep->d_type != DT_REG) {
      continue;
    }
    char* entry = ep->d_name;
    [arr addObject:[NSString stringWithCString:entry encoding:NSASCIIStringEncoding]];
    
  }
  closedir(dp);
  free(bundle_root);
  
  return arr;
}

@interface ViewController ()

@end

id vc;
NSArray* bundle_pocs;

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  vc = self;

  // get the list of poc binaries:
  bundle_pocs = getBundlePocs();
  
  self.picker.dataSource = self;
  self.picker.delegate = self;

  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^(void){
    do_exploit();
    dispatch_async(dispatch_get_main_queue(), ^{
      _getPSButton.enabled = true;
      _execButton.enabled = true;
    });    
  });

}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

// number of columns in picker
- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
  return 1;
}

// The number of rows of data
- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
  return [bundle_pocs count];
}

- (NSString*)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
  return [bundle_pocs objectAtIndex:row];
}

- (void)logMsg:(NSString*)msg {
  NSLog(@"%@", msg);
  NSString* line = [msg stringByAppendingString:@"\n"];
  dispatch_async(dispatch_get_main_queue(), ^{
    _statusTextView.text = [_statusTextView.text stringByAppendingString:line];
  });
}

// the button's initial state is disabled; it's only enabled when the exploit is done
//- (IBAction)getPSButtonClicked:(id)sender {
- (IBAction)getPSButtonClicked:(UIBarButtonItem *)sender {
  char* ps_output = ps();
  logMsg(ps_output);
}

- (IBAction)execButtonClicked:(id)sender {
  // is there at least one poc?
  if ([bundle_pocs count] == 0) {
    return;
  }
  
  // get the currently selected poc index
  NSInteger row = [_picker selectedRowInComponent:0];

  // what's the name of that poc?
  NSString* poc_string = [bundle_pocs objectAtIndex:row];
  [self logMsg:poc_string];
  // do this on a different queue?
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^(void){
    run_poc([poc_string cStringUsingEncoding:NSASCIIStringEncoding]);
  });
}
@end

// c method to log string
void logMsg(char* msg) {
  NSString* str = [NSString stringWithCString:msg encoding:NSASCIIStringEncoding];
  [vc logMsg:str];
}
