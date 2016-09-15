//
//  ViewController.h
//  Arduino Door Lock
//
//  Created by Brian Team on 9/15/15.
//  Copyright (c) 2015 Brian Team. All rights reserved.
//

#import <UIKit/UIKit.h>

//necessary for viewing wifi SSID
@import SystemConfiguration.CaptiveNetwork;

@interface LockUnlockViewController : UIViewController <NSURLConnectionDelegate>
{
    bool isLocked;
    NSMutableData *responseData;
    NSString *serverString;
    NSString *currentSSID;
    NSString *pastSSID;
}

@property (nonatomic, retain) IBOutlet UILabel *lockStatusLabel;
@property (nonatomic, retain) IBOutlet UIButton *toggleLockButton;
@property (nonatomic, retain) IBOutlet UIImageView *lockImageView;
@property (nonatomic, retain) IBOutlet UIButton *refreshButton;
@property (nonatomic, retain) UIActivityIndicatorView *spinner;
@property (nonatomic, retain) UIView *loadingView;


-(IBAction)toggleLockAction:(id)sender;
-(IBAction)refreshLockStatusAction:(id)sender;

-(void)applicationEnteredForeground;
-(void)getLockStatus;
-(void)parseResponse:(NSString *)response;
-(void)sendToggleCommand;
-(NSDictionary *)fetchSSIDInfo;

//NSURLConnectionDelegate methods
-(void)connectionDidFinishLoading:(NSURLConnection *)connection;
-(void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data;
-(void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response;
-(void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error;

@end

