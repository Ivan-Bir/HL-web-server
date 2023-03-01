FROM nginx

# WORKDIR /var/www/html

COPY nginx.conf /etc/nginx/nginx.conf
COPY  ./httptest /var/www/html/httptest

