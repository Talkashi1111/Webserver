#!/usr/bin/env perl
use strict;
use warnings;
use HTTP::Tiny;   # core since Perl 5.14
use JSON::PP;     # core since Perl 5.14

my $http = HTTP::Tiny->new(
    timeout => 5,
    agent   => "perl-weather/1.0"
);

# ------------------------------------------------------------
# 1.  Figure out the city from IP (ipinfo.io)   ---------------
# ------------------------------------------------------------
my $loc_city = '';

my $resp = $http->get('https://ipinfo.io/json');
if ( $resp->{success} ) {
    my $info = JSON::PP::decode_json( $resp->{content} );
    $loc_city = $info->{city} // '';
}

# Allow override from the command line (e.g. perl weather.pl Zurich)
$loc_city = $ARGV[0] if @ARGV;

# ------------------------------------------------------------
# 2.  Build wttr.in URL  -------------------------------------
# ------------------------------------------------------------
my $url = 'https://wttr.in/';
$url .= $loc_city if $loc_city ne '';
$url .= '?format=3';        # "City: ☀️  +21 °C"

# ------------------------------------------------------------
# 3.  Fetch and display weather  ------------------------------
# ------------------------------------------------------------
my $weather = $http->get($url);
die "Unable to fetch weather: $weather->{status} $weather->{reason}\n"
    unless $weather->{success};

my $weather_text = $weather->{content};

# HTML output with embedded CSS
print "Content-Type: text/html; charset=UTF-8\r\n\r\n";
print <<"HTML";
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Weather Information</title>
    <link rel="stylesheet" href="../style/weather.css">
    <style>
        nav {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            background-color: #444;
            padding: 10px;
            z-index: 100;
        }

        nav ul {
            list-style-type: none;
            display: flex;
            gap: 15px;
            margin: 0;
            padding: 0 20px;
        }

        nav ul li {
            margin: 0;
        }

        nav ul li a {
            color: #fff;
            text-decoration: none;
            padding: 5px 15px;
            border-radius: 5px;
            transition: background-color 0.3s;
        }

        nav ul li a:hover {
            background-color: #666;
        }

        /* Adjust body to account for fixed nav */
        body {
            padding-top: 50px;
        }
    </style>
</head>
<body>
    <nav>
        <ul>
            <li><a href="../index.html">Home</a></li>
        </ul>
    </nav>
    <div class="container">
        <div class="weather-card">
            <h1>Current Weather</h1>
            <div class="weather-data">$weather_text</div>
            <div class="footer">
                <p>Data provided by wttr.in</p>
                <p class="timestamp">Updated: @{[scalar localtime]}</p>
            </div>
        </div>
    </div>
</body>
</html>
HTML
#TODO: add a link to home page
