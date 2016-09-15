//
//  ViewController.m
//  Arduino Door Lock
//
//  Created by Brian Team on 9/15/15.
//  Copyright (c) 2015 Brian Team. All rights reserved.
//

#import "LockUnlockViewController.h"
#import "Reachability.h"

@interface LockUnlockViewController()

@end

@implementation LockUnlockViewController
static NSString const  *password = @"testpass";
static NSString const *username = @"testuser";

- (void)viewWillAppear:(BOOL)animated {
    self.lockStatusLabel.text = @"Loading...";
    self.toggleLockButton.userInteractionEnabled = NO;
    self.toggleLockButton.enabled = NO;
    
    [self applicationEnteredForeground];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationEnteredForeground) name:UIApplicationWillEnterForegroundNotification object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(reachabilityDidChange) name:@"reachabilityChangedNotification" object:nil];
    
    responseData = [NSMutableData data];
    }

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void)applicationDidEnterBackground:(UIApplication *) application {
    pastSSID = currentSSID;
}

- (void)applicationEnteredForeground {
    NSDictionary *ssidDict = [self fetchSSIDInfo];
    if (ssidDict != nil) {
        currentSSID = [ssidDict objectForKey:@"SSID"];
        NSLog(@"%s: Got SSID; on network: %@", __func__, currentSSID);
        if ([currentSSID isEqualToString: @"UVASAE"] || [currentSSID isEqualToString:@"THH/MLR"]) {
            serverString = [NSString stringWithFormat:@"http://10.0.1.11/"];
        } else {
            serverString = [NSString stringWithFormat:@"http://73.171.7.172:8081/"];
            //serverString = [NSString stringWithFormat:@"http://secondstring.ddns.net"];
        }
    } else {
        NSLog(@"%s: Got no SSID info, setting default server to external IP", __func__);
        //serverString = [NSString stringWithFormat:@"http://secondstring.ddns.net"];
        serverString = [NSString stringWithFormat:@"http://73.171.7.172:8081/"];
    }
    
    [self refreshLockStatusAction:nil];
}

- (void)reachabilityDidChange:(NSNotification *)notification {
    Reachability *reach = [notification object];
    
    if ([reach isReachableViaWiFi]) {
        NSLog(@"Network connection changed to wifi");
    } else if ([reach isReachableViaWWAN]) {
        NSLog(@"Network connection changed to LTE");
    } else {
        NSLog(@"Lost network connection!");
    }
}

- (IBAction)refreshLockStatusAction:(id)sender {
    self.lockStatusLabel.text = @"Loading...";
    self.toggleLockButton.enabled = NO;
    self.toggleLockButton.userInteractionEnabled = NO;
    
    float centerX = self.view.bounds.size.width / 2;
    float centerY = self.view.bounds.size.height / 2;
    self.loadingView = [[UIView alloc] initWithFrame:CGRectMake(centerX - 60, centerY - 60, 120, 120)];
    self.loadingView.layer.cornerRadius = 15;
    self.loadingView.opaque = NO;
    self.loadingView.backgroundColor = [UIColor colorWithWhite:0.0f alpha:0.6f];
    [self.view addSubview:self.loadingView];
                                   
    self.spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
    self.spinner.frame = CGRectMake(self.loadingView.bounds.size.width / 2 - 20, self.loadingView.bounds.size.height / 2 - 20, 40, 40);
    [self.spinner startAnimating];
    [self.loadingView addSubview:self.spinner];
    
    [self getLockStatus];
}

- (IBAction)toggleLockAction:(id)sender {
    self.toggleLockButton.enabled = NO;
    self.toggleLockButton.userInteractionEnabled = NO;
    [self sendToggleCommand];
}

- (void)sendToggleCommand {
    NSLog(@"Sending toggle request to %@", [serverString stringByAppendingString:[NSString stringWithFormat: @"?username=%@&password=%@&toggle=1~", username, password]]);
    //building request by adding on correct toggle params to base server string
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:[[serverString stringByAppendingString:[NSString stringWithFormat: @"?username=%@&password=%@&toggle=1~", username, password]] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]]];
    
    [request setHTTPMethod:@"GET"];
    [NSURLConnection connectionWithRequest:request delegate:self];

}

- (void) getLockStatus {
    NSLog(@"Sending get status request to %@", [serverString stringByAppendingString:[NSString stringWithFormat:@"?username=%@&password=%@~", username, password]]);
    //building request by adding on correct get status params to base server string
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:[[serverString stringByAppendingString:[NSString stringWithFormat:@"?username=%@&password=%@~", username, password]] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]]];
    
    [request setHTTPMethod:@"GET"];
    [NSURLConnection connectionWithRequest:request delegate:self];
}

- (void) parseResponse:(NSString *)response {
    NSRange colonIndex = [response rangeOfString:@":"];
    if (colonIndex.location != NSNotFound) {
        NSString *lockedStr = [response substringFromIndex:(colonIndex.location + 1)];
        lockedStr = [lockedStr substringToIndex:1];
        if ([lockedStr isEqualToString:@"1"] || [lockedStr isEqualToString:@"0"]) {
            NSLog(@"Request success; lock status is '%@'", lockedStr);
            isLocked = [lockedStr boolValue];
            self.toggleLockButton.enabled = YES;
            self.toggleLockButton.userInteractionEnabled = YES;
            
            //changing labels and uiimages
            if (isLocked) {
                self.lockStatusLabel.text = @"Locked";
                [self.lockImageView setImage:[UIImage imageNamed:@"lockedImage"]];
            } else {
                [self.lockImageView setImage:[UIImage imageNamed:@"unlockedImage"]];
                self.lockStatusLabel.text = @"Unlocked!";
            }
        } else {
            //failed authentification
            NSLog(@"Lock status get failed! Status recieved is '%@'", lockedStr);
        }
    }
}

- (NSDictionary *) fetchSSIDInfo {
    NSArray *interfaceNames = (__bridge_transfer NSArray *)CNCopySupportedInterfaces();
    NSLog(@"%s: Supported interfaces: %@", __func__, interfaceNames);
    
    NSDictionary *ssidInfo;
    for (NSString *interfaceName in interfaceNames) {
        ssidInfo = (__bridge_transfer NSDictionary *)CNCopyCurrentNetworkInfo((__bridge CFStringRef)interfaceName);
        NSLog(@"%s: %@ => %@", __func__, interfaceName, ssidInfo);
        
        if (ssidInfo && [ssidInfo count]) {
            break;
        }
    }
    return ssidInfo;
}


#pragma mark - Connection Functions

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
    //NSDictionary *headers = [(NSHTTPURLResponse *)response allHeaderFields];
    [responseData setLength:0];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
    [responseData appendData:data];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
    [self.spinner removeFromSuperview];
    [self.loadingView removeFromSuperview];
    NSString *responseString = [[NSString alloc] initWithData:responseData encoding:NSUTF8StringEncoding];
    NSLog(@"Server response recieved: %@", responseString);
    if (![responseString isEqualToString:@""]) {
        [self parseResponse:responseString];
    } else {
        //no response recieved
    }
    
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
    NSLog(@"Connection Failed: %@", [error description]);
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Error" message:@"Connection to server failed. Please try again." delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil, nil];
    [alert show];
    
}

#pragma mark

@end
