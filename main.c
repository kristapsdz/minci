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

static void
get_print_time(struct khtmlreq *req, int64_t start, int64_t given)
{

	if (given == 0) {
		khtml_attrx(req, KELEM_TD, KATTR_CLASS,
			KATTRX_STRING, "fail", KATTR__MAX);
		khtml_closeelem(req, 1);
		return;
	} 

	khtml_attrx(req, KELEM_TD, KATTR_CLASS,
		KATTRX_STRING, "success", KATTR__MAX);
	khtml_attrx(req, KELEM_TIME,
		KATTR_DATETIME, KATTRX_INT, given, KATTR__MAX);
	khtml_int(req, given - start);
	khtml_closeelem(req, 1);
	khtml_closeelem(req, 1);
}

static void
get_print_report_header(struct khtmlreq *req)
{

	khtml_elem(req, KELEM_TR);
	khtml_elem(req, KELEM_TH);
	khtml_puts(req, "project");
	khtml_closeelem(req, 1);
	khtml_elem(req, KELEM_TH);
	khtml_puts(req, "system");
	khtml_closeelem(req, 1);
	khtml_elem(req, KELEM_TH);
	khtml_puts(req, "date");
	khtml_closeelem(req, 1);
	khtml_elem(req, KELEM_TH);
	khtml_puts(req, "precheck");
	khtml_closeelem(req, 1);
	khtml_elem(req, KELEM_TH);
	khtml_puts(req, "depends");
	khtml_closeelem(req, 1);
	khtml_elem(req, KELEM_TH);
	khtml_puts(req, "build");
	khtml_closeelem(req, 1);
	khtml_elem(req, KELEM_TH);
	khtml_puts(req, "regress");
	khtml_closeelem(req, 1);
	khtml_elem(req, KELEM_TH);
	khtml_puts(req, "install");
	khtml_closeelem(req, 1);
	khtml_elem(req, KELEM_TH);
	khtml_puts(req, "distcheck");
	khtml_closeelem(req, 1);
	khtml_closeelem(req, 1);
}

static void
get_print_report(const struct report *p, void *arg)
{
	struct khtmlreq	*req = arg;
	struct tm	 tm;

	memset(&tm, 0, sizeof(struct tm));
	KUTIL_EPOCH2TM(p->start, &tm);

	khtml_elem(req, KELEM_TR);

	khtml_elem(req, KELEM_TD);
	khtml_puts(req, p->project.name);
	khtml_closeelem(req, 1);

	khtml_elem(req, KELEM_TD);
	khtml_attrx(req, KELEM_TIME,
		KATTR_DATETIME, KATTRX_INT, p->start, KATTR__MAX);
	khtml_int(req, tm.tm_year + 1900);
	khtml_puts(req, "-");
	khtml_int(req, tm.tm_mon + 1);
	khtml_puts(req, "-");
	khtml_int(req, tm.tm_mday);
	khtml_closeelem(req, 1);
	khtml_closeelem(req, 1);

	khtml_elem(req, KELEM_TD);
	khtml_puts(req, p->unames);
	khtml_puts(req, " ");
	khtml_puts(req, p->unamer);
	khtml_closeelem(req, 1);

	get_print_time(req, p->start, p->env);
	get_print_time(req, p->env, p->depend);
	get_print_time(req, p->depend, p->build);
	get_print_time(req, p->build, p->test);
	get_print_time(req, p->test, p->install);
	get_print_time(req, p->install, p->distcheck);
	khtml_closeelem(req, 1);
}

static void
get_html(struct kreq *r)
{
	struct khtmlreq	 req;

	http_open(r, KHTTP_200, r->mime);

	khtml_open(&req, r, 0);
	khtml_elem(&req, KELEM_HTML);
	khtml_elem(&req, KELEM_HEAD);
	khtml_elem(&req, KELEM_STYLE);
	khtml_puts(&req, "td.success { background-color: rgba(0, 255, 0, 0.2); }");
	khtml_puts(&req, "td.fail { background-color: rgba(255, 0, 0, 0.5); }");
	khtml_puts(&req, "th, td { padding: 0.5rem 1rem; }");
	khtml_puts(&req, "table { border-spacing: unset; }");
	khtml_closeelem(&req, 1);
	khtml_closeelem(&req, 1);
	khtml_elem(&req, KELEM_BODY);
	khtml_elem(&req, KELEM_TABLE);
	khtml_elem(&req, KELEM_THEAD);
	get_print_report_header(&req);
	khtml_closeelem(&req, 1);
	khtml_elem(&req, KELEM_TBODY);
	db_report_iterate_last(r->arg, get_print_report, &req);
	khtml_closeelem(&req, 1);
	khtml_closeelem(&req, 1);
	khtml_closeelem(&req, 1);
	khtml_closeelem(&req, 1);
	khtml_close(&req);
}

static void
get(struct kreq *r)
{

	return get_html(r);
}

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

	/* Signature must be 32-byte string. */

	for (sig = NULL, i = 0; i < r->fieldsz; i++)
		if (strcmp(r->fields[i].key, "signature") == 0 &&
		    kvalid_stringne(&r->fields[i]) &&
		    r->fields[i].valsz == 32) {
			sig = &r->fields[i];
			break;
		}

	/* Check our input is sane. */

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
		return;
	}

	/* 
	 * If stages fail, subsequent must also fail. 
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
		return;
	}

	/* Time must increase. */

	if ((kpe->parsed.i != 0 && kpe->parsed.i < kps->parsed.i) ||
	    (kpd->parsed.i != 0 && kpd->parsed.i < kpe->parsed.i) ||
	    (kpb->parsed.i != 0 && kpb->parsed.i < kpd->parsed.i) ||
	    (kpt->parsed.i != 0 && kpt->parsed.i < kpb->parsed.i) ||
	    (kpi->parsed.i != 0 && kpi->parsed.i < kpt->parsed.i) ||
	    (kpc->parsed.i != 0 && kpc->parsed.i < kpi->parsed.i)) {
		kutil_warnx(r, NULL, "invalid timestamp sequence");
		http_open(r, KHTTP_403, KMIME__MAX);
	}

	/* Hash log digest (may be zero-length). */

	MD5Init(&ctx);
	MD5Update(&ctx, kpl->parsed.s, kpl->valsz);
	MD5End(&ctx, logdigest);

	/* Get the project. */

	proj = db_project_get_byname(r->arg,
		kpn->parsed.s); /* name */
	if (proj == NULL) {
		kutil_warnx(r, NULL, "invalid project");
		http_open(r, KHTTP_403, KMIME__MAX);
		goto out;
	}

	/* Get the user. */

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
