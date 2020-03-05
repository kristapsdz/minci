/*	$Id$ */
/*
 * Copyright (c) 2020 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/types.h>

#include <assert.h>
#include <err.h>
#include <inttypes.h>
#include <md5.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <kcgi.h>
#include <kcgihtml.h>

#include "extern.h"

enum	page {
	PAGE_INDEX,
	PAGE__MAX
};

struct	req {
	struct kreq	*r;
	struct khtmlreq	 html;
};

static const char *const pages[PAGE__MAX] = {
	"index", /* PAGE_INDEX */
};

static void
http_open(struct kreq *r, enum khttp code, enum kmime mime)
{

	khttp_head(r, kresps[KRESP_STATUS], 
		"%s", khttps[code]);
	khttp_head(r, kresps[KRESP_CACHE_CONTROL], 
		"%s", "no-cache, no-store, must-revalidate");
	khttp_head(r, kresps[KRESP_PRAGMA], 
		"%s", "no-cache");
	khttp_head(r, kresps[KRESP_EXPIRES], 
		"%s", "0");
	if (mime != KMIME__MAX)
		khttp_head(r, kresps[KRESP_CONTENT_TYPE], 
			"%s", kmimetypes[mime]);
	khttp_body(r);
}

/*
 * Print a block with a time offset of "start" to "given".
 * If "given" is zero, suppress any content printing.
 */
static void
gen_html_offs(struct khtmlreq *req,
	const char *classes, int64_t start, int64_t given)
{

	khtml_attr(req, KELEM_DIV,
		KATTR_CLASS, classes, KATTR__MAX);
	if (given != 0) {
		khtml_attrx(req, KELEM_TIME, KATTR_DATETIME,
			KATTRX_INT, given, KATTR__MAX);
		khtml_int(req, given - start);
		khtml_closeelem(req, 1); /* time */
	}
	khtml_closeelem(req, 1); /* div */
}

/*
 * Print the first row of a report table.
 */
static void
gen_html_last_header(struct khtmlreq *req)
{

	khtml_attr(req, KELEM_DIV, 
		KATTR_CLASS, "row", KATTR__MAX);
	khtml_attr(req, KELEM_DIV, KATTR_CLASS, 
		"header report-id", KATTR__MAX);
	khtml_closeelem(req, 1); /* cell */
	khtml_attr(req, KELEM_DIV, KATTR_CLASS, 
		"header project-name", KATTR__MAX);
	khtml_closeelem(req, 1); /* cell */
	khtml_attr(req, KELEM_DIV, KATTR_CLASS, 
		"header report-start", KATTR__MAX);
	khtml_closeelem(req, 1); /* cell */
	khtml_attr(req, KELEM_DIV, KATTR_CLASS, 
		"header report-system", KATTR__MAX);
	khtml_closeelem(req, 1); /* cell */
	khtml_attr(req, KELEM_DIV, KATTR_CLASS, 
		"header report-env", KATTR__MAX);
	khtml_closeelem(req, 1); /* cell */
	khtml_attr(req, KELEM_DIV, KATTR_CLASS, 
		"header report-deps", KATTR__MAX);
	khtml_closeelem(req, 1); /* cell */
	khtml_attr(req, KELEM_DIV, KATTR_CLASS, 
		"header report-build", KATTR__MAX);
	khtml_closeelem(req, 1); /* cell */
	khtml_attr(req, KELEM_DIV, KATTR_CLASS, 
		"header report-regress", KATTR__MAX);
	khtml_closeelem(req, 1); /* cell */
	khtml_attr(req, KELEM_DIV, KATTR_CLASS, 
		"header report-install", KATTR__MAX);
	khtml_closeelem(req, 1); /* cell */
	khtml_attr(req, KELEM_DIV, KATTR_CLASS, 
		"header report-dist", KATTR__MAX);
	khtml_closeelem(req, 1); /* cell */
	khtml_closeelem(req, 1); /* row */
}

/*
 * Print a record as it would appear in an HTML table.
 */
static void
get_html_last_report(const struct report *p, void *arg)
{
	struct req	*r = arg;
	struct tm	 tm;
	char		*url;

	url = kutil_urlpartx(NULL,
		r->r->pname, 
		ksuffixes[KMIME_TEXT_HTML],
		pages[PAGE_INDEX],
		valid_keys[VALID_REPORT_ID].name,
		KATTRX_INT, p->id, NULL);
	memset(&tm, 0, sizeof(struct tm));
	KUTIL_EPOCH2TM(p->start, &tm);

	khtml_attr(&r->html, KELEM_DIV,
		KATTR_CLASS, "row", KATTR__MAX);

	khtml_attr(&r->html, KELEM_DIV, KATTR_CLASS, 
		"cell report-id", KATTR__MAX);
	khtml_attr(&r->html, KELEM_A,
		KATTR_HREF, url, KATTR__MAX);
	khtml_int(&r->html, p->id);
	khtml_closeelem(&r->html, 1); /* link */
	khtml_closeelem(&r->html, 1); /* cell */

	khtml_attr(&r->html, KELEM_DIV, KATTR_CLASS, 
		"cell project-name", KATTR__MAX);
	khtml_puts(&r->html, p->project.name);
	khtml_closeelem(&r->html, 1); /* cell */

	khtml_attr(&r->html, KELEM_DIV, KATTR_CLASS, 
		"cell report-start", KATTR__MAX);
	khtml_attrx(&r->html, KELEM_TIME,
		KATTR_DATETIME, KATTRX_INT, p->start, KATTR__MAX);
	khtml_int(&r->html, tm.tm_year + 1900);
	khtml_puts(&r->html, "-");
	khtml_int(&r->html, tm.tm_mon + 1);
	khtml_puts(&r->html, "-");
	khtml_int(&r->html, tm.tm_mday);
	khtml_closeelem(&r->html, 1); /* time */
	khtml_closeelem(&r->html, 1); /* cell */

	khtml_attr(&r->html, KELEM_DIV, KATTR_CLASS, 
		"cell report-system", KATTR__MAX);
	khtml_puts(&r->html, p->unames);
	khtml_puts(&r->html, " ");
	khtml_puts(&r->html, p->unamer);
	khtml_closeelem(&r->html, 1); /* cell */

	gen_html_offs(&r->html, "cell "
		"report-env", p->start, p->env);
	gen_html_offs(&r->html, "cell "
		"report-deps", p->env, p->depend);
	gen_html_offs(&r->html, "cell "
		"report-build", p->depend, p->build);
	gen_html_offs(&r->html, "cell "
		"report-regress", p->build, p->test);
	gen_html_offs(&r->html, "cell "
		"report-install", p->test, p->install);
	gen_html_offs(&r->html, "cell "
		"report-dist", p->install, p->distcheck);

	khtml_closeelem(&r->html, 1); /* row */
	free(url);
}

/*
 * List a single record as text/html.
 * Outputs HTTP 404 (error) or 200 (success).
 */
static void
get_html_single(struct kreq *r)
{
	struct khtmlreq	 req;
	struct report	*p;
	struct kpair	*kp;
	char		 buf[64];

	kp = r->fieldmap[VALID_REPORT_ID];
	assert(kp != NULL);
	p = db_report_get_byid(r->arg, 
		kp->parsed.i); /* id */
	if (p == NULL) {
		http_open(r, KHTTP_404, KMIME__MAX);
		return;
	}

	http_open(r, KHTTP_200, r->mime);

	khtml_open(&req, r, 0);
	khtml_elem(&req, KELEM_HTML);
	khtml_elem(&req, KELEM_HEAD);
	khtml_attrx(&req, KELEM_LINK, 
		KATTR_REL, KATTRX_STRING, "stylesheet",
		KATTR_HREF, KATTRX_STRING, "/minci.css",
		KATTR__MAX);
	khtml_closeelem(&req, 1); /* head */
	khtml_elem(&req, KELEM_BODY);

	khtml_elem(&req, KELEM_DIV);
	khtml_int(&req, p->id);
	khtml_closeelem(&req, 1); /* div */

	khtml_elem(&req, KELEM_DIV);
	khtml_puts(&req, p->project.name);
	khtml_closeelem(&req, 1); /* div */

	khtml_elem(&req, KELEM_DIV);
	khtml_attrx(&req, KELEM_TIME,
		KATTR_DATETIME, KATTRX_INT, 
		p->start, KATTR__MAX);
	kutil_epoch2str(p->start, buf, sizeof(buf));
	khtml_puts(&req, buf);
	khtml_closeelem(&req, 1); /* time */
	khtml_closeelem(&req, 1); /* div */

	khtml_elem(&req, KELEM_DIV);
	khtml_puts(&req, p->unames);
	khtml_puts(&req, " ");
	khtml_puts(&req, p->unamer);
	khtml_closeelem(&req, 1); /* div */

	gen_html_offs(&req, "report-env", p->start, p->env);
	gen_html_offs(&req, "report-deps", p->env, p->depend);
	gen_html_offs(&req, "report-build", p->depend, p->build);
	gen_html_offs(&req, "report-regress", p->build, p->test);
	gen_html_offs(&req, "report-install", p->test, p->install);
	gen_html_offs(&req, "report-dist", p->install, p->distcheck);

	khtml_closeelem(&req, 1); /* body */
	khtml_closeelem(&req, 1); /* html */
	khtml_close(&req);
	db_report_free(p);
}

/*
 * List the last *n* records, sorted by time of accept.
 * Always outputs HTTP 200.
 */
static void
gen_html_last(struct kreq *r)
{
	struct req	 req;

	req.r = r;
	http_open(r, KHTTP_200, r->mime);
	khtml_open(&req.html, r, 0);

	khtml_elem(&req.html, KELEM_HTML);
	khtml_elem(&req.html, KELEM_HEAD);
	khtml_attrx(&req.html, KELEM_LINK, 
		KATTR_REL, KATTRX_STRING, "stylesheet",
		KATTR_HREF, KATTRX_STRING, "/minci.css",
		KATTR__MAX);
	khtml_closeelem(&req.html, 1); /* head */
	khtml_elem(&req.html, KELEM_BODY);
	khtml_attr(&req.html, KELEM_DIV, 
		KATTR_CLASS, "table", KATTR__MAX);
	gen_html_last_header(&req.html);
	db_report_iterate_last(r->arg, 
		get_html_last_report, &req);
	khtml_closeelem(&req.html, 1); /* table */
	khtml_closeelem(&req.html, 1); /* body */
	khtml_closeelem(&req.html, 1); /* html */
	khtml_close(&req.html);
}

/*
 * List one or more records as text/html.
 */
static void
get(struct kreq *r)
{

	if (r->fieldmap[VALID_REPORT_ID] != NULL)
		get_html_single(r);
	else
		gen_html_last(r);
}

/*
 * Process a record submission.
 * Records are signed into a non-ORT field "signature".
 * This performs all sanity checks: failure is sequentially consistent,
 * timestamps are increasing, etc.
 * It outputs only HTTP 403 (error) and 201 (success).
 */
static void
post(struct kreq *r)
{
	struct project	*proj = NULL;
	struct user	*user = NULL;
	struct kpair	*kps, *kpe, *kpd, *kpb, *kpt,
			*kpi, *kpc, *kpn, *kpl, *sig,
			*kpu, *kpum, *kpun, *kpur, *kpus,
			*kpuv;
	size_t		 i, sz;
	MD5_CTX		 ctx;
	char		*buf = NULL;
	char		 digest[MD5_DIGEST_STRING_LENGTH],
			 logdigest[MD5_DIGEST_STRING_LENGTH];

	/* 
	 * Check our non-ORT signature field was given.
	 * It must be a 32-byte string.
	 */

	for (sig = NULL, i = 0; i < r->fieldsz; i++)
		if (strcmp(r->fields[i].key, "signature") == 0 &&
		    kvalid_stringne(&r->fields[i]) &&
		    r->fields[i].valsz == 32) {
			sig = &r->fields[i];
			break;
		}

	/* Check our ORT fields were given. */

	if (sig == NULL ||
	    (kpn = r->fieldmap[VALID_PROJECT_NAME]) == NULL ||
	    (kpd = r->fieldmap[VALID_REPORT_DEPEND]) == NULL ||
	    (kpc = r->fieldmap[VALID_REPORT_DISTCHECK]) == NULL ||
	    (kpe = r->fieldmap[VALID_REPORT_ENV]) == NULL ||
	    (kpi = r->fieldmap[VALID_REPORT_INSTALL]) == NULL ||
	    (kpl = r->fieldmap[VALID_REPORT_LOG]) == NULL ||
	    (kps = r->fieldmap[VALID_REPORT_START]) == NULL ||
	    (kpb = r->fieldmap[VALID_REPORT_BUILD]) == NULL ||
	    (kpt = r->fieldmap[VALID_REPORT_TEST]) == NULL ||
	    (kpum = r->fieldmap[VALID_REPORT_UNAMEM]) == NULL ||
	    (kpun = r->fieldmap[VALID_REPORT_UNAMEN]) == NULL ||
	    (kpur = r->fieldmap[VALID_REPORT_UNAMER]) == NULL ||
	    (kpus = r->fieldmap[VALID_REPORT_UNAMES]) == NULL ||
	    (kpuv = r->fieldmap[VALID_REPORT_UNAMEV]) == NULL ||
	    (kpu = r->fieldmap[VALID_USER_APIKEY]) == NULL) {
		kutil_warnx(r, NULL, "invalid request");
		http_open(r, KHTTP_403, KMIME__MAX);
		goto out;
	}

	/* 
	 * Check that if stages fail, subsequent must also fail. 
	 * Also, the log should only be specified on failure.
	 */

	if ((kpe->parsed.i == 0 &&
	     (kpd->parsed.i != 0 ||
	      kpb->parsed.i != 0 ||
	      kpt->parsed.i != 0 ||
	      kpi->parsed.i != 0 ||
	      kpc->parsed.i != 0)) ||
	    (kpd->parsed.i == 0 &&
	     (kpb->parsed.i != 0 ||
	      kpt->parsed.i != 0 ||
	      kpi->parsed.i != 0 ||
	      kpc->parsed.i != 0)) ||
	    (kpb->parsed.i == 0 &&
	     (kpt->parsed.i != 0 ||
	      kpi->parsed.i != 0 ||
	      kpc->parsed.i != 0)) ||
	    (kpt->parsed.i == 0 &&
	     (kpi->parsed.i != 0 ||
	      kpc->parsed.i != 0)) ||
	    (kpi->parsed.i == 0 &&
	     (kpc->parsed.i != 0)) ||
	    (kpc->parsed.i != 0 && kpl->valsz)) {
		kutil_warnx(r, NULL, "invalid stages");
		http_open(r, KHTTP_403, KMIME__MAX);
		goto out;
	}

	/* 
	 * Check that times must increase.
	 * FIXME: the minci.sh script uses `date +%s` to create
	 * timestamps, which of course aren't guaranteed to increase
	 * (though it's unlikely they won't).
	 */

	if ((kpe->parsed.i != 0 && kpe->parsed.i < kps->parsed.i) ||
	    (kpd->parsed.i != 0 && kpd->parsed.i < kpe->parsed.i) ||
	    (kpb->parsed.i != 0 && kpb->parsed.i < kpd->parsed.i) ||
	    (kpt->parsed.i != 0 && kpt->parsed.i < kpb->parsed.i) ||
	    (kpi->parsed.i != 0 && kpi->parsed.i < kpt->parsed.i) ||
	    (kpc->parsed.i != 0 && kpc->parsed.i < kpi->parsed.i)) {
		kutil_warnx(r, NULL, "invalid timestamp sequence");
		http_open(r, KHTTP_403, KMIME__MAX);
		goto out;
	}

	/* Hash log digest (may be zero-length). */

	MD5Init(&ctx);
	MD5Update(&ctx, kpl->parsed.s, kpl->valsz);
	MD5End(&ctx, logdigest);

	/* Get the project and user. */

	proj = db_project_get_byname(r->arg,
		kpn->parsed.s); /* name */
	if (proj == NULL) {
		kutil_warnx(r, NULL, "invalid project");
		http_open(r, KHTTP_403, KMIME__MAX);
		goto out;
	}

	user = db_user_get_bykey(r->arg,
		kpu->parsed.i); /* apikey */
	if (user == NULL) {
		kutil_warnx(r, NULL, "invalid user");
		http_open(r, KHTTP_403, KMIME__MAX);
		goto out;
	}

	/* 
	 * Re-create the signature with the user's secret key.
	 * This authenticates the message.
	 */

	sz = (size_t)kasprintf(&buf,
		"project-name=%s&"
		"report-build=%" PRId64 "&"
		"report-distcheck=%" PRId64 "&"
		"report-env=%" PRId64 "&"
		"report-depend=%" PRId64 "&"
		"report-install=%" PRId64 "&"
		"report-log=%s&"
		"report-start=%" PRId64 "&"
		"report-test=%" PRId64 "&"
		"report-unamem=%s&"
		"report-unamen=%s&"
		"report-unamer=%s&"
		"report-unames=%s&"
		"report-unamev=%s&"
		"user-apisecret=%s",
		proj->name,
		kpb->parsed.i,
		kpc->parsed.i,
		kpe->parsed.i,
		kpd->parsed.i,
		kpi->parsed.i,
		logdigest,
		kps->parsed.i,
		kpt->parsed.i,
		kpum->parsed.s,
		kpun->parsed.s,
		kpur->parsed.s,
		kpus->parsed.s,
		kpuv->parsed.s,
		user->apisecret);

	MD5Init(&ctx);
	MD5Update(&ctx, buf, sz);
	MD5End(&ctx, digest);

	if (strcasecmp(digest, sig->parsed.s)) {
		kutil_warnx(r, NULL, "bad signature");
		http_open(r, KHTTP_403, KMIME__MAX);
		goto out;
	}

	/* Insert the record. */

	db_report_insert(r->arg,
		proj->id, /* projectid */
		user->id, /* userid */
		kps->parsed.i, /* start */
		kpe->parsed.i, /* env */
		kpd->parsed.i, /* depend */
		kpb->parsed.i, /* build */
		kpt->parsed.i, /* test */
		kpi->parsed.i, /* install */
		kpc->parsed.i, /* distcheck */
		time(NULL), /* ctime */
		kpl == NULL ? "" : kpl->parsed.s, /* log */
		kpum->parsed.s, /* unamem */
		kpun->parsed.s, /* unamen */
		kpur->parsed.s, /* unamer */
		kpus->parsed.s, /* unames */
		kpuv->parsed.s); /* unamev */

	kutil_info(r, user->email, "log submitted: %s", proj->name);
	http_open(r, KHTTP_201, KMIME__MAX);
out:
	db_project_free(proj);
	db_user_free(user);
	free(buf);
}

int
main(void)
{
	struct kreq	 r;
	enum kcgi_err	 er;

	/* Basic checks: parse and valid page. */

	er = khttp_parse(&r, valid_keys,
		VALID__MAX, pages, PAGE__MAX, PAGE_INDEX);

	if (er != KCGI_OK)
		kutil_errx(&r, NULL, 
			"khttp_parse: %s", kcgi_strerror(er));

	if (r.page == PAGE__MAX) {
		http_open(&r, KHTTP_404, KMIME__MAX);
		khttp_free(&r);
		return EXIT_SUCCESS;
	}

	if ((r.arg = db_open_logging
	    (DATADIR "/minci.db", NULL, warnx, NULL)) == NULL) {
		kutil_errx(&r, NULL, "db_open: %s", 
			DATADIR "/minci.db");
		khttp_free(&r);
		return EXIT_SUCCESS;
	}

	if (pledge("stdio", NULL) == -1) {
		kutil_warn(NULL, NULL, "pledge");
		db_close(r.arg);
		khttp_free(&r);
		return EXIT_FAILURE;
	}

	if (r.method == KMETHOD_POST) {
		db_role(r.arg, ROLE_producer);
		post(&r);
	} else {
		db_role(r.arg, ROLE_consumer);
		get(&r);
	}

	db_close(r.arg);
	khttp_free(&r);
	return EXIT_SUCCESS;
}
