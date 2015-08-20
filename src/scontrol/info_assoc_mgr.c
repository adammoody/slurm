/*****************************************************************************\
 *  info_assoc_mgr.c - Association Manager information from the
 *                     slurmctld functions for scontrol.
 *****************************************************************************
 *  Copyright (C) 2004 CSCS
 *  Copyright (C) 2015 SchedMD LLC
 *  Written by Stephen Trofinoff and Danny Auble
 *
 *  This file is part of SLURM, a resource management program.
 *  For details, see <http://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#include "scontrol.h"

static uint32_t tres_cnt = 0;
static char **tres_names = NULL;

static void _print_tres_line(const char *name, uint64_t *limits, uint64_t *used,
			     uint64_t divider, bool last)
{
	int i;
	char *next_line = last ? "\n" : "\n    ";
	char *new_line_char = one_liner && !last ? " " : next_line;
	bool comma = 0;

	xassert(tres_cnt);
	xassert(tres_names);

	printf("%s=", name);
	if (!limits)
		goto endit;

	for (i=0; i<tres_cnt; i++) {
		if (limits[i] == INFINITE64)
			continue;

		printf("%s%s=%"PRIu64,
		       comma ? "," : "", tres_names[i], limits[i]);

		if (used) {
			uint64_t total_used = used[i];

			if (divider)
				total_used /= divider;

			printf("(%"PRIu64")", total_used);
		}

		comma = 1;
	}
endit:
	printf("%s", new_line_char);
}

static void _print_assoc_mgr_info(const char *name, assoc_mgr_info_msg_t *msg)
{
	ListIterator itr;
	slurmdb_user_rec_t *user_rec;
	slurmdb_assoc_rec_t *assoc_rec;
	slurmdb_qos_rec_t *qos_rec;
	uint64_t tmp64_array[msg->tres_cnt];
	char *new_line_char = one_liner ? " " : "\n    ";
	int i;

	printf("Current Association Manager state\n");

	tres_cnt = msg->tres_cnt;
	tres_names = msg->tres_names;

	if (!msg->user_list || !list_count(msg->user_list)) {
		printf("\nNo users currently cached in Slurm.\n\n");
	} else {
		printf("\nUser Records\n\n");

		itr = list_iterator_create(msg->user_list);
		while ((user_rec = list_next(itr))) {
			printf("UserName=%s(%u) DefAccount=%s "
			       "DefWckey=%s AdminLevel=%s\n",
			       user_rec->name,
			       user_rec->uid,
			       user_rec->default_acct,
			       user_rec->default_wckey,
			       slurmdb_admin_level_str(user_rec->admin_level));
		}
		list_iterator_destroy(itr);
	}

	if (!msg->assoc_list || !list_count(msg->assoc_list)) {
		printf("\nNo associations currently cached in Slurm.\n\n");
	} else {
		printf("\nAssociation Records\n\n");

		itr = list_iterator_create(msg->assoc_list);
		while ((assoc_rec = list_next(itr))) {
			if (!assoc_rec->usage)
				continue;

			printf("ClusterName=%s Account=%s ",
			       assoc_rec->cluster,
			       assoc_rec->acct);

			if (assoc_rec->user)
				printf("UserName=%s(%u) ",
				       assoc_rec->user,
				       assoc_rec->uid);
			else
				printf("UserName= ");

			printf("Partition=%s ID=%u%s",
			       assoc_rec->partition ? assoc_rec->partition : "",
			       assoc_rec->id,
			       new_line_char);

			printf("SharesRaw/Norm/Level/Factor="
			       "%u/%.2f/%u/%.2f%s",
			       assoc_rec->shares_raw,
			       assoc_rec->usage->shares_norm,
			       assoc_rec->usage->level_shares,
			       assoc_rec->usage->fs_factor,
			       new_line_char);

			printf("UsageRaw/Norm/Efctv=%.2Lf/%.2Lf/%.2Lf%s",
			       assoc_rec->usage->usage_raw,
			       assoc_rec->usage->usage_norm,
			       assoc_rec->usage->usage_efctv,
			       new_line_char);

			if (assoc_rec->parent_acct)
				printf("ParentAccount=%s(%u) ",
				       assoc_rec->parent_acct,
				       assoc_rec->parent_id);
			else
				printf("ParentAccount= ");

			printf("Lft-Rgt=%u-%u DefAssoc=%s%s",
			       assoc_rec->lft,
			       assoc_rec->rgt,
			       assoc_rec->is_def ? "Yes" : "No",
			       new_line_char);


			if (assoc_rec->grp_jobs != INFINITE)
				printf("GrpJobs=%u(%u)",
				       assoc_rec->grp_jobs,
				       assoc_rec->usage->used_jobs);
			else
				printf("GrpJobs=");
			/* NEW LINE */
			printf("%s", new_line_char);

			if (assoc_rec->grp_submit_jobs != INFINITE)
				printf("GrpSubmitJobs=%u(%u) ",
				       assoc_rec->grp_submit_jobs,
				       assoc_rec->usage->used_submit_jobs);
			else
				printf("GrpSubmitJobs= ");
			if (assoc_rec->grp_wall != INFINITE)
				printf("GrpWall=%u(%.2f)",
				       assoc_rec->grp_wall,
				       assoc_rec->usage->grp_used_wall);
			else
				printf("GrpWall=");
			/* NEW LINE */
			printf("%s", new_line_char);

			_print_tres_line("GrpTRES",
					 assoc_rec->grp_tres_ctld,
					 assoc_rec->usage->grp_used_tres, 0, 0);
			if (assoc_rec->usage->usage_tres_raw)
				for (i=0; i<tres_cnt; i++)
					tmp64_array[i] = (uint64_t)
						assoc_rec->usage->
						usage_tres_raw[i];
			else
				memset(tmp64_array, 0, sizeof(tmp64_array));
			_print_tres_line("GrpTRESMins",
					 assoc_rec->grp_tres_mins_ctld,
					 tmp64_array, 60, 0);
			_print_tres_line("GrpTRESRunMins",
					 assoc_rec->grp_tres_run_mins_ctld,
					 assoc_rec->usage->
					 grp_used_tres_run_secs, 60, 0);

			if (assoc_rec->max_jobs != INFINITE)
				printf("MaxJobs=%u(%u) ",
				       assoc_rec->max_jobs,
				       assoc_rec->usage->used_jobs);
			else
				printf("MaxJobs= ");

			if (assoc_rec->max_submit_jobs != INFINITE)
				printf("MaxSubmitJobs=%u(%u) ",
				       assoc_rec->max_submit_jobs,
				       assoc_rec->usage->used_submit_jobs);
			else
				printf("MaxSubmitJobs= ");

			if (assoc_rec->max_wall_pj != INFINITE)
				printf("MaxWallPJ=%u",
				       assoc_rec->max_wall_pj);
			else
				printf("MaxWallPJ=");

			/* NEW LINE */
			printf("%s", new_line_char);

			_print_tres_line("MaxTRESPJ",
					 assoc_rec->max_tres_ctld,
					 NULL, 0, 0);

			_print_tres_line("MaxTRESPN",
					 assoc_rec->max_tres_pn_ctld,
					 NULL, 0, 0);

			_print_tres_line("MaxTRESMinsPJ",
					 assoc_rec->max_tres_mins_ctld,
					 NULL, 0, 1);

			/* Doesn't do anything yet */
			/* _print_tres_line("MaxTRESRunMins", */
			/* 		 assoc_rec->max_tres_mins_ctld, */
			/* 		 NULL, 0, 1); */
		}
	}

	if (!msg->qos_list || !list_count(msg->qos_list)) {
		printf("\nNo QOS currently cached in Slurm.\n\n");
	} else {

		printf("\nQOS Records\n\n");

		itr = list_iterator_create(msg->qos_list);
		while ((qos_rec = list_next(itr))) {
			if (!qos_rec->usage)
				continue;

			printf("QOS=%s(%u)%s", qos_rec->name, qos_rec->id,
				new_line_char);

			printf("UsageRaw=%Lf%s",
			       qos_rec->usage->usage_raw,
			       new_line_char);

			if (qos_rec->grp_jobs != INFINITE)
				printf("GrpJobs=%u(%u) ",
				       qos_rec->grp_jobs,
				       qos_rec->usage->grp_used_jobs);
			else
				printf("GrpJobs= ");
			if (qos_rec->grp_submit_jobs != INFINITE)
				printf("GrpSubmitJobs=%u(%u) ",
				       qos_rec->grp_submit_jobs,
				       qos_rec->usage->grp_used_submit_jobs);
			else
				printf("GrpSubmitJobs= ");
			if (qos_rec->grp_wall != INFINITE)
				printf("GrpWall=%u(%.2f)",
				       qos_rec->grp_wall,
				       qos_rec->usage->grp_used_wall);
			else
				printf("GrpWall=");
			/* NEW LINE */
			printf("%s", new_line_char);

			_print_tres_line("GrpTRES",
					 qos_rec->grp_tres_ctld,
					 qos_rec->usage->grp_used_tres, 0, 0);
			if (qos_rec->usage->usage_tres_raw)
				for (i=0; i<tres_cnt; i++)
					tmp64_array[i] = (uint64_t)
						qos_rec->usage->
						usage_tres_raw[i];
			else
				memset(tmp64_array, 0, sizeof(tmp64_array));
			_print_tres_line("GrpTRESMins",
					 qos_rec->grp_tres_mins_ctld,
					 tmp64_array, 60, 0);
			_print_tres_line("GrpTRESRunMins",
					 qos_rec->grp_tres_run_mins_ctld,
					 qos_rec->usage->
					 grp_used_tres_run_secs, 60, 0);

			if (qos_rec->max_jobs_pu != INFINITE)
				printf("MaxJobsPU=%u(%u) ",
				       qos_rec->max_jobs_pu,
				       qos_rec->usage->grp_used_jobs);
			else
				printf("MaxJobs= ");

			if (qos_rec->max_submit_jobs_pu != INFINITE)
				printf("MaxSubmitJobs=%u(%u) ",
				       qos_rec->max_submit_jobs_pu,
				       qos_rec->usage->grp_used_submit_jobs);
			else
				printf("MaxSubmitJobs= ");

			if (qos_rec->max_wall_pj != INFINITE)
				printf("MaxWallPJ=%u",
				       qos_rec->max_wall_pj);
			else
				printf("MaxWallPJ=");

			/* NEW LINE */
			printf("%s", new_line_char);

			_print_tres_line("MaxTRESPJ",
					 qos_rec->max_tres_pj_ctld,
					 NULL, 0, 0);

			_print_tres_line("MaxTRESPN",
					 qos_rec->max_tres_pn_ctld,
					 NULL, 0, 0);

			_print_tres_line("MaxTRESPU",
					 qos_rec->max_tres_pu_ctld,
					 NULL, 0, 0);

			_print_tres_line("MaxTRESMinsPJ",
					 qos_rec->max_tres_mins_pj_ctld,
					 NULL, 0, 0);

			/* Doesn't do anything yet */
			/* _print_tres_line("MaxTRESRunMinsPU", */
			/* 		 qos_rec->max_tres_mins_pu_ctld, */
			/* 		 NULL, 0); */

			_print_tres_line("MinTRESPJ",
					 qos_rec->min_tres_pj_ctld,
					 NULL, 0, 1);
		}
	}
}

/* scontrol_print_assoc_mgr_info()
 *
 * Retrieve and display the association manager information
 * from the controller
 *
 */

extern void scontrol_print_assoc_mgr_info(const char *name)
{
	int cc;
	assoc_mgr_info_request_msg_t req;
	assoc_mgr_info_msg_t *msg = NULL;

	/* FIXME: add more filtering in the future */
	memset(&req, 0, sizeof(assoc_mgr_info_request_msg_t));
	req.flags = ASSOC_MGR_INFO_FLAG_ASSOC | ASSOC_MGR_INFO_FLAG_USERS |
		ASSOC_MGR_INFO_FLAG_QOS;
	if (name) {
		req.user_list = list_create(NULL);
		list_append(req.user_list, (char *)name);
	}
	/* call the controller to get the meat */
	cc = slurm_load_assoc_mgr_info(&req, &msg);

	FREE_NULL_LIST(req.user_list);

	if (cc != SLURM_PROTOCOL_SUCCESS) {
		/* Hosed, crap out. */
		exit_code = 1;
		if (quiet_flag != 1)
			slurm_perror("slurm_load_assoc_mgr_info error");
		return;
	}

	/* print the info
	 */
	_print_assoc_mgr_info(name, msg);

	/* free at last
	 */
	slurm_free_assoc_mgr_info_msg(msg);

	return;
}
