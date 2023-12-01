#!/usr/bin/awk

BEGIN{
FS=":";
success=0;
requests=0;
timeout=0;
total_response = 0.0;
}
{	
	if ($1 ~ /Total response time/) total_response +=$2;
	if ($1 ~ /Number of requests/) requests +=$2;
	if ($1 ~ /Successful responses/) success +=$2;
	if ($1 ~ /Timeouts/) timeout+=$2;
}

END{
average_response = (total_response/requests);

printf("%f ",average_response);
printf("%f ",requests);
printf("%f ",success);
printf("%f\n",timeout);
}
