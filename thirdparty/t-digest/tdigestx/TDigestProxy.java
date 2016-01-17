package tdigestx;

import com.tdunning.math.stats.TDigest;


public class TDigestProxy
{
	public static final int DEFAULT_DIGEST_TYPE = 0;
	public static final int ARRAY_DIGEST_TYPE = 1;
	public static final int AVLTREE_DIGEST_TYPE = 2;
	public static final int TREE_DIGEST_TYPE = 3;

	public static final int DEFAULT_COMPRESSION = 100;


	private TDigest tdigest = null;


	public static int getDefaultDigestType()
	{
		return DEFAULT_DIGEST_TYPE;
	}

	public static int getArrayDigestType()
	{
		return ARRAY_DIGEST_TYPE;
	}

	public static int getAvlTreeDigestType()
	{
		return AVLTREE_DIGEST_TYPE;
	}

	public static int getTreeDigestType()
	{
		return TREE_DIGEST_TYPE;
	}

	public static int getDefaultCompression()
	{
		return DEFAULT_COMPRESSION;
	}

	public void init()
	{
		init(DEFAULT_DIGEST_TYPE, DEFAULT_COMPRESSION);
	}

	public void init(int digestType)
	{
		init(digestType, DEFAULT_COMPRESSION);
	}

	public void init(int digestType, double compression)
	{
		switch (digestType)
		{
			case ARRAY_DIGEST_TYPE:
				tdigest = TDigest.createArrayDigest(compression);
				break;
			case AVLTREE_DIGEST_TYPE:
				tdigest = TDigest.createAvlTreeDigest(compression);
				break;
			case DEFAULT_DIGEST_TYPE:
				tdigest = TDigest.createDigest(compression);
				break;
			case TREE_DIGEST_TYPE:
				tdigest = TDigest.createTreeDigest(compression);
				break;
			default:
				throw new IllegalArgumentException("Unknown digest type");
		}
	}

	public void add(double x)
	{
		tdigest.add(x);
	}

	public void add(double x, int w)
	{
		tdigest.add(x, w);
	}

	public double quantile(double q)
	{
		return tdigest.quantile(q);
	}
}
