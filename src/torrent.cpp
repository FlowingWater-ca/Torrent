#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "main_lib.h"

typedef enum
{
	TZERO  = 1 << 7,
	TSTART = 0,
	TUDP   = 1,
	THTTP  = 2,
	TANN   = 3,
	TTOR   = 4,
	TTRA   = 5,
	TCOM   = 6,
	TORG   = 7,
	TOPEN  = 8,
	TMAX   = 9
} TR_ENUM;

const char *tlist[TMAX] =
{
	"&tr=",
	"udp",
	"http",
	"announce",
	"torrent",
	"tracker",
	".com",
	".org",
	"open"
};

typedef struct
{
	bool operator()(const char *left, const char *right) const
	{
		return strcmp(left, right) < 0;
	}
} CStringComp;

std::map<const char *, uint8_t, CStringComp> KEYWORDS[] = {
	// Keyword 1
	{
		{"Other", 1},
		{"Games", 2}
	},
	// Keyword 2
	{
		{"Other", 1}
	},
	{
		{"Other", 1},
		{"PC", 2}
	}
};

constexpr uint8_t KEYWORDS_LIM[] = {
	3,
	2,
	3
};

// Reverse keyword lookup
const char *KEYWORDS_R0[] = {
	NULL,
	"Other",
	"Games"
};
const char *KEYWORDS_R1[] = {
	NULL,
	"Other"
};
const char *KEYWORDS_R2[] = {
	NULL,
	"Other",
	"PC"
};

const char **KEYWORDS_R[] = {
	// Keyword 1
	KEYWORDS_R0,
	// Keyword 2
	KEYWORDS_R1,
	KEYWORDS_R2
};

uint32_t compressTracker(uint8_t *tr)
{
	char buf3[3] = {0, 0, 0};
	uint32_t ret = 0, len = 0, i, j, mark;
	if (!(len = u8len(tr))) return 0;

	for (i = 0; i < len; ++i, ++ret)
	{
		if (ret >= MAGNET_TR_LEN) [[unlikely]] return 0;
		tr[ret] = tr[i];
		if (tr[i] == '%')
		{
			tr[ret] = TZERO;
			for (mark = ret; i < len && tr[i] == '%'; i += 3)
			{
				if (i + 2 >= len) return 0;
				memcpy(buf3, tr + i + 1, 2);

				tr[mark]++;
				if (tr[mark] > MAX_U8 - TMAX) return 0;
				errno = 0;
				tr[++ret] = (uint8_t) strtol(buf3, NULL, 16);
				if (errno != 0) return 0;
			}
			--i;
		}
		else
		{
			for (j = 0; j < TMAX; ++j)
			{
				if ((mark = u8cmp(tr + i, (char *) (tlist[j]))))
				{
					tr[ret] = MAX_U8 - j;
					i += mark - 1;
					break;
				}
			}
		}
	}
	tr[ret] = 0;

	return ret;
}

uint32_t decompressTracker(uint8_t *tr, char ret[MAGNET_TR_LEN])
{
	int i, j;
	uint32_t retLen = 0;

	for (i = 0; tr[i]; ++i)
	{
		if (tr[i] < TZERO)
		{
			if (retLen + 1 >= MAGNET_TR_LEN) return 0;
			ret[retLen++] = tr[i];
		}
		else if (tr[i] <= MAX_U8 - TMAX)
		{
			for (j = tr[i++] - TZERO; j > 0; ++i, --j)
			{
				if (retLen + 3 >= MAGNET_TR_LEN) return 0;
				retLen += sprintf(ret + retLen, "%%%02X", tr[i]);
			}
			--i;
		}
		else
		{
			if (retLen + strlen(tlist[MAX_U8 - tr[i]]) >= MAGNET_TR_LEN) return 0;
			retLen += sprintf(ret + retLen, tlist[MAX_U8 - tr[i]]);
		}
	}
	ret[retLen] = 0;
	return retLen;
}

void printTorCat(torDB *target)
{
	if (!target) return;
	for (uint32_t i = 1; i < KEYWORDS_LIM[0]; ++i)
	{
		printf("%s: ", KEYWORDS_R[0][i]);
		for (uint32_t u32 : target->cat[i].idx) printf("%u ", u32);
		for (uint32_t j = 1; j < KEYWORDS_LIM[i]; ++j)
		{
			printf("\n\t%s: ", KEYWORDS_R[i][j]);
			for (uint32_t u32 : target->cat[i].sub[j]) printf("%u ", u32);
		}
		printf("\n");
	}
}

bool processPack(torDB *td, pack *px)
{
	uint8_t kt1, kt2;
	uint32_t idx;
	if (!td || !px) return false;
	if (!isKeywordValid(px->kt)) return false;

	idx = td->pak.size() - 1;
	kt1 = px->kt & MAX_U4;
	kt2 = px->kt >> 4;

	if (!kt2) td->cat[kt1].idx.push_back(idx);
	else td->cat[kt1].sub[kt2].push_back(idx);

	return true;
}

bool newPack(torDB *td, uint8_t xt[MAGNET_XT_LEN], uint64_t xl, char *dn, uint8_t *tr, uint8_t kt)
{
	uint32_t ndn = 0, ntr = 0;
	pack px = {.crc = {0}, .xt = {0}, .xl = 0ULL, .dn = NULL, .tr = NULL, .kt = 0};
	if (!td || !xt || !dn || !tr) goto cleanup;

	if (!(ndn = strlen(dn)) || ndn > MAGNET_DN_LEN) goto cleanup;
	if (!(ntr = u8len(tr)) || ntr > MAGNET_TR_LEN) goto cleanup;
	if (!(ntr = compressTracker(tr))) goto cleanup;

	memcpy(px.xt, xt, MAGNET_XT_LEN);
	px.xl = xl;
	if (!(px.dn = (char *) calloc(ndn + 1, 1))) goto cleanup;
	memcpy(px.dn, dn, ndn);
	if (!(px.tr = (uint8_t *) calloc(ntr + 1, 1))) goto cleanup;
	memcpy(px.tr, tr, ntr);

	if (isKeywordValid(kt)) px.kt = kt;
	else goto cleanup;

	if (!checkPack(&px, true)) goto cleanup;
	td->pak.push_back(px);
	if (!processPack(td, &px)) printf("proc pack failed %u %u\n", kt & MAX_U4, kt >> 4);

	return true;
cleanup:
	printf("\nfailed new pack kt[i] %s< ndn %u ntr %u %p kt: %u %u\n",
		   dn, ndn, ntr, tr, kt & MAX_U4, kt >> 4);
	if (px.dn) free(px.dn);
	if (px.tr) free(px.tr);
	return false;
}

bool isKeywordValid(uint8_t kt, uint8_t *kt1p, uint8_t *kt2p)
{
	uint8_t kt1 = kt & MAX_U4, kt2 = kt >> 4;
	if (kt1p) *kt1p = kt1;
	if (kt2p) *kt2p = kt2;
	if (kt1 == 0 || kt1 > KEYWORDS_LIM[0]) return false;
	if (kt2 > KEYWORDS_LIM[kt1]) return false;
	return true;
}

bool lookupKeyword(uint8_t kt, char **kt1, char **kt2)
{
	uint8_t kt1u, kt2u;
	if (!kt1 || !kt2 || !isKeywordValid(kt, &kt1u, &kt2u)) return false;
	*kt1 = (char *) KEYWORDS_R[0][kt1u];
	if (kt2u > 0) *kt2 = (char *) KEYWORDS_R[kt1u][kt2u];
	else *kt2 = NULL;
	return true;
}

uint8_t lookupKeyword(const char *kt1, const char *kt2)
{
	uint8_t ret = 0;
	std::map<const char *, uint8_t>::iterator it;

	if (!kt1) return ret;
	if ((it = KEYWORDS[0].find(kt1)) == KEYWORDS[0].end()) return ret;
	ret = it->second & MAX_U4;

	if (!kt2) return ret;
	if ((it = KEYWORDS[ret].find(kt2)) == KEYWORDS[ret].end()) return ret;
	ret |= (it->second & MAX_U4) << 4;

	return ret;
}

std::vector<uint32_t> searchTorDB(torDB *td, const char *kt1, const char *kt2, const char *str)
{
	uint8_t kt1u, kt2u;
	std::vector<uint32_t> result;

	kt1u = lookupKeyword(kt1, kt2);
	kt2u = kt1u >> 4;
	kt1u &= MAX_U4;

	if (kt1u)
	{
		result.insert(result.begin(), td->cat[kt1u].idx.begin(), td->cat[kt1u].idx.end());
		if (kt2u)
			result.insert(result.begin(), td->cat[kt1u].sub[kt2u].begin(), td->cat[kt1u].sub[kt2u].end());
	}
	return result;
}

static inline void deletePack(pack *target)
{
	if (!target) return;
	if (target->dn) free(target->dn);
	if (target->tr) free(target->tr);
}

torDB::torDB()
{
	cat.resize(KEYWORDS_LIM[0]);
	for (uint8_t i = 1; i < KEYWORDS_LIM[0]; ++i) cat[i].sub.resize(KEYWORDS_LIM[i]);
}

torDB::~torDB()
{
	for (uint64_t i = 0; i < pak.size(); ++i) deletePack(&(pak[i]));
}
