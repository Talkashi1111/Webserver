#!/usr/bin/perl
use strict;
use warnings;
use POSIX qw(strftime);    # for the Date: header
use Net::SMTP;             # core since Perl 5.8

# ------------------------------------------------------------------
# 0.  SMTP settings (read from env, fall back to demo defaults)
# ------------------------------------------------------------------
my $SMTP_HOST = $ENV{SMTP_HOST} // 'smtp.example.com';
my $SMTP_PORT = $ENV{SMTP_PORT} // 587;            # 587 = STARTTLS
my $SMTP_USER = $ENV{SMTP_USER} // 'you@example.com';
my $SMTP_PASS = $ENV{SMTP_PASS} // 'password';     # leave blank → no AUTH
# ------------------------------------------------------------------
# 1.  Read the raw POST body
# ------------------------------------------------------------------
my $len = $ENV{CONTENT_LENGTH} // 0;
my $body = '';
read STDIN, $body, $len;

# ------------------------------------------------------------------
# 2.  Decode application/x-www-form-urlencoded   (no CGI.pm)
# ------------------------------------------------------------------
my %param;

foreach my $pair ( split /&/, $body ) {
    next unless length $pair;                 # skip empty fragments

    my ($k, $v) = split /=/, $pair, 2;
    $k //= '';
    $v //= '';

    # + → space,  %XX → byte   (do it for both scalars explicitly)
    for ($k, $v) {
        tr/+/ /;
        s/%([0-9A-Fa-f]{2})/chr hex $1/eg;
    }

    $param{$k} = $v;
}   # <-- ONE and only one closing brace ends the foreach block
# ------------------------------------------------------------------
# 3.  Extract & sanitise fields
# ------------------------------------------------------------------
for my $fld ( qw(from to subject) ) {
    $param{$fld} //= '';
    $param{$fld} =~ s/[\r\n]//g;            # strip CR/LF (header-inject)
}

my $from    = $param{from}    || 'you@example.com';
my $to      = $param{to}      || 'friend@example.com';
my $subject = $param{subject} || 'Website message';
my $text    = $param{body}    || '— empty body —';

# ------------------------------------------------------------------
# 4.  Compose a minimal RFC-5322 message
# ------------------------------------------------------------------
my $date = strftime '%a, %d %b %Y %H:%M:%S %z', gmtime;
my $msg  = <<"EOF";
From: $from
To: $to
Subject: $subject
Date: $date
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit

$text
EOF

# ------------------------------------------------------------------
# 5.  Send via Net::SMTP  (needs IO::Socket::SSL for STARTTLS)
# ------------------------------------------------------------------
my $ok = eval {
    my $smtp = Net::SMTP->new(
        $SMTP_HOST,
        Port    => $SMTP_PORT,
        Timeout => 15,
        Debug   => 0,
    ) or die "SMTP connect failed\n";

    # Upgrade to TLS if offered
    $smtp->starttls() if $smtp->supports('STARTTLS');

    # Authenticate only if creds supplied
    if ( length $SMTP_USER && length $SMTP_PASS ) {
        $smtp->auth( $SMTP_USER, $SMTP_PASS )
          or die "AUTH failed: " . $smtp->message;
    }

    $smtp->mail($from)          or die "MAIL FROM failed: " . $smtp->message;
    $smtp->to($to)              or die "RCPT TO failed : " . $smtp->message;
    $smtp->data()               or die "DATA failed    : " . $smtp->message;
    $smtp->datasend($msg);
    $smtp->dataend()            or die "DATAEND failed : " . $smtp->message;
    $smtp->quit;
    1;                                              # success flag
};

# ------------------------------------------------------------------
# 6.  Return HTML to the browser
# ------------------------------------------------------------------
print "Content-Type: text/html\r\n\r\n";

if ($ok) {
    print <<'HTML';
<!DOCTYPE html><html><head><meta charset="utf-8"><title>Mail sent</title></head>
<body style="font-family:sans-serif">
  <h1>Mail sent!</h1>
  <p>Thank you for your message.</p>
  <p><a href="/pages/mail.html">Back to form</a></p>
</body></html>
HTML
} else {
    ( my $err = $@ ) =~ s/[<&>]/sprintf '&#x%X;', ord($&)/ge;  # HTML-escape
    print <<"HTML";
<!DOCTYPE html><html><head><meta charset="utf-8"><title>Mail failed</title></head>
<body style="font-family:sans-serif">
  <h1>Mail failed</h1>
  <pre>$err</pre>
  <p><a href="/pages/mail.html">Back to form</a></p>
</body></html>
HTML
}
