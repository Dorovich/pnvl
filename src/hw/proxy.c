/* proxy.c - External access to Proto-NVLink memory
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "proxy.h"
#include "pnvl.h"
#include "qapi/qapi-commands-machine.h"

/* ============================================================================
 * Private
 * ============================================================================
 */

static void pnvl_proxy_init_server(PNVLDevice *dev)
{
	PNVLProxy *proxy = &dev->proxy;
	socklen_t len = sizeof(proxy->client.addr);

	if (bind(proxy->server.sockd, (struct sockaddr *)&proxy->server.addr,
				sizeof(proxy->server.addr)) < 0) {
		perror("bind");
		return;
	}

	if (listen(proxy->server.sockd, PNVL_PROXY_MAXQ) < 0) {
		perror("listen");
		return;
	}

	printf("Server started, waiting for client...\n");

	proxy->client.sockd = accept(proxy->server.sockd,
			(struct sockaddr *)&proxy->client.addr, &len);
	if (proxy->client.sockd < 0) {
		perror("accept");
		return;
	}

	/* Begin connection test */
	if (pnvl_proxy_issue_req(dev, PNVL_REQ_ACK) != PNVL_SUCCESS) {
		perror("pnvl_proxy_issue_req");
		return;
	}
	if (pnvl_proxy_await_req(dev, PNVL_REQ_ACK) != PNVL_SUCCESS) {
		perror("pnvl_proxy_await_req");
		return;
	}
	puts("Client connection established.");
	/* End connection test */
}

static void pnvl_proxy_init_client(PNVLDevice *dev)
{
	PNVLProxy *proxy = &dev->proxy;

	if (connect(proxy->server.sockd, (struct sockaddr *)&proxy->server.addr,
				sizeof(proxy->server.addr)) < 0) {
		perror("connect");
		return;
	}

	/* Begin connection test */
	if (pnvl_proxy_await_req(dev, PNVL_REQ_ACK) != PNVL_SUCCESS) {
		perror("pnvl_proxy_await_req");
		return;
	}
	if (pnvl_proxy_issue_req(dev, PNVL_REQ_ACK) != PNVL_SUCCESS) {
		perror("pnvl_proxy_issue_req");
		return;
	}
	puts("Server connection established.");
	/* End connection test */
}

static inline int pnvl_proxy_endpoint(PNVLDevice *dev)
{
	return (dev->proxy.server_mode ?
			dev->proxy.client.sockd : dev->proxy.server.sockd);
}

static ProxyRequest pnvl_proxy_wait_req(PNVLDevice *dev)
{
	int con = pnvl_proxy_endpoint(dev);
	ProxyRequest req = PNVL_REQ_NIL;
	fd_set cons;

	FD_ZERO(&cons);
	FD_SET(con, &cons);
	if (select(con+1, &cons, NULL, NULL, NULL) && FD_ISSET(con, &cons))
		recv(con, &req, sizeof(req), 0);

	return req;
}

static int pnvl_proxy_handle_req(PNVLDevice *dev, ProxyRequest req)
{
	int con = pnvl_proxy_endpoint(dev);

	switch(req) {
	case PNVL_REQ_SYN:
		pnvl_execute(dev);
		break;
	case PNVL_REQ_RST:
		qmp_system_reset(NULL); /* see qemu/ui/gtk.c L1313 */
		break;
	case PNVL_REQ_SLN:
		pnvl_proxy_issue_req(dev, PNVL_REQ_RLN);
		send(con, &dev->dma.config.len_avail,
				sizeof(dev->dma.config.len_avail), 0);
		break;
	case PNVL_REQ_RLN:
		recv(con, &dev->dma.config.len_avail,
				sizeof(dev->dma.config.len_avail), 0);
		break;
	case PNVL_REQ_ACK:
		break;
	default:
		return PNVL_FAILURE;
	}

	return PNVL_SUCCESS;
}

/* ============================================================================
 * Public
 * ============================================================================
 */

int pnvl_proxy_issue_req(PNVLDevice *dev, ProxyRequest req)
{
	int ret, con = pnvl_proxy_endpoint(dev);

	ret = send(con, &req, sizeof(req), 0);
	if (ret < 0)
		return PNVL_FAILURE;

	return PNVL_SUCCESS;
}

int pnvl_proxy_wait_and_handle_req(PNVLDevice *dev)
{
	return pnvl_proxy_handle_req(dev, pnvl_proxy_wait_req(dev));
}

int pnvl_proxy_await_req(PNVLDevice *dev, ProxyRequest req)
{
	ProxyRequest new_req;

	do {
		new_req = pnvl_proxy_wait_req(dev);
	} while(new_req != PNVL_FAILURE && new_req != req);

	return pnvl_proxy_handle_req(dev, new_req);
}

/*
 * Receive page: buffer <-- socket
 */
int pnvl_proxy_rx_page(PNVLDevice *dev, uint8_t *buff)
{
	int ret, src = pnvl_proxy_endpoint(dev);
	int len = 0;

	ret = recv(src, &len, sizeof(len), 0);
	if (ret < 0 || !len)
		return PNVL_FAILURE;

	ret = recv(src, buff, len, 0);
	if (ret < 0)
		return PNVL_FAILURE;

	return len;
}

/*
 * Transmit page: buffer --> socket
 */
int pnvl_proxy_tx_page(PNVLDevice *dev, uint8_t *buff, int len)
{
	int ret, dst = pnvl_proxy_endpoint(dev);

	if (len <= 0)
		return PNVL_FAILURE;

	ret = send(dst, &len, sizeof(len), 0);
	if (ret < 0)
		return PNVL_FAILURE;

	ret = send(dst, buff, len, 0);
	if (ret < 0)
		return PNVL_FAILURE;

	return PNVL_SUCCESS;
}

bool pnvl_proxy_get_mode(Object *obj, Error **errp)
{
	PNVLDevice *dev = PNVL(obj);
	return dev->proxy.server_mode;
}

void pnvl_proxy_set_mode(Object *obj, bool mode, Error **errp)
{
	PNVLDevice *dev = PNVL(obj);
	dev->proxy.server_mode = mode;
}

void pnvl_proxy_reset(PNVLDevice *dev)
{
	return;
}

void pnvl_proxy_init(PNVLDevice *dev, Error **errp)
{
	PNVLProxy *proxy = &dev->proxy;
	struct hostent *h;

	h = gethostbyname(PNVL_PROXY_HOST);
	if (!h) {
		herror("gethostbyname");
		return;
	}

	proxy->server.sockd = socket(AF_INET, SOCK_STREAM, 0);
	if (proxy->server.sockd < 0) {
		perror("socket");
		return;
	}

	bzero(&proxy->server.addr, sizeof(proxy->server.addr));
	proxy->server.addr.sin_family = AF_INET;
	proxy->server.addr.sin_port = htons(proxy->port);
	proxy->server.addr.sin_addr.s_addr = *(in_addr_t *)h->h_addr_list[0];

	if (proxy->server_mode)
		pnvl_proxy_init_server(dev);
	else
		pnvl_proxy_init_client(dev);
}

void pnvl_proxy_fini(PNVLDevice *dev)
{
	if (dev->proxy.server_mode)
		close(dev->proxy.client.sockd);
	close(dev->proxy.server.sockd);
}
