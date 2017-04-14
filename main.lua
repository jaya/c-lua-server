resource = string.match(request, "GET /(%S+)");
respond("You asked for " .. resource);
