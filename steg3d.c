#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
//#define EPSILON 	0.000100
//#define DECODE_EPSILON	10000
#define EPSILON 	0.001000
#define DECODE_EPSILON	1000
#define METHOD_DIST	1
#define DEBUG		printf("%s : %d\n", __func__, __LINE__); // Debug

static char		*error(char *str)
{
	printf("Error: %s\n", str);
	return (NULL);
}

#define READ_SIZE	512
char		*readalltext(char *filename, size_t *size)
{
	int		fd;
	char		buf[READ_SIZE];
	char		*data;
	int		sz[2];

	if (!(fd = open(filename, O_RDONLY)))
		return (error("File open"));
	if (!(data = malloc(READ_SIZE)))
		return (error("Heap"));
	sz[1] = 0;
	while ((sz[0] = read(fd, buf, READ_SIZE)) > 0)
	{
		if (!(data = realloc(data, sz[1] + sz[0])))
			return (error("Heap"));
		memcpy(data + sz[1], buf, sz[0]);
		sz[1] += sz[0];
	}
	if (sz[0] < 0)
	{
		free(data);
		return (error("Read"));
	}
	if (size)
		*size = sz[1];
	return (data);
}


typedef unsigned int		uint;

typedef struct s_point
{
	float		x;
	float 		y;
	float 		z;
	float 		w;
	int		index;
}		t_point;

#define MAX_FACE_POINT		6
typedef struct s_face
{
	uint		num_points;
	//t_point		**p;
	t_point		*p[MAX_FACE_POINT];
	uint		pIndex[MAX_FACE_POINT];
}		t_face;

typedef struct s_obj
{
	uint		num_points;
	t_point		*p;
	uint		num_faces;
	t_face		*f;
}		t_obj;

static uint		count_verb(char *obj, char verb)
{
	uint		num;

	if (!obj)
		return (0);
	num = 0;
	while (*obj)
	{
		if (*obj == '\n' || *obj == verb)
		{
			while (*obj && (*obj == ' ' || *obj == '\n'))
				obj++;
			if (obj[0] == verb && (obj[1] == ' ' || obj[1] == '\t'))
				num++;
		}
		obj++;
	}
	return (num);
}

static int		isNumericFloat(char c)
{
	return ((c >= '0' && c <= '9') || c == '.' || c == '-' ? 1 : 0);
}


int		obj_parse_points(t_obj *obj, char *data)
{
	int		index;

	if (!obj || !data)
		return (1);
	if (obj->p)
	{
		free(obj->p);
		obj->p = NULL;
	}
	if ((obj->num_points = count_verb(data, 'v')))
		if (!(obj->p = malloc(sizeof(struct s_point) * obj->num_points)))
			return (1);
	index = 0;
	while (*data)
	{
		if (*data == '\n' || *data == 'v')
		{
			while (*data && (*data == ' ' || *data == '\n'))
				data++;
			if (data[0] == 'v' && (data[1] == ' ' || data[1] == '\t'))
			{
				while (*data && !isNumericFloat(*data))
					data++;
				obj->p[index].x = atof(data++);
				while (*data && isNumericFloat(*data))
					data++;
				while (*data && !isNumericFloat(*data))
					data++;
				obj->p[index].y = atof(data++);
				while (*data && isNumericFloat(*data))
					data++;
				while (*data && !isNumericFloat(*data))
					data++;
				obj->p[index].z = atof(data++);
				obj->p[index].w = 0.0;
				obj->p[index].index = index;
				index++;
			}
		}
		data++;
	}
	return (index != obj->num_points ? 1 : 0);
}

static int		isNumeric(char c)
{
	return (c >= '0' && c <= '9' ? 1 : 0);
}

static uint	count_face_points(char *obj)
{
	uint		count;

	count = 0;
	while (*obj && *obj != '\n')
	{
		while (*obj && !isNumeric(*obj) && *obj != '\n')
			obj++;
		if (isNumeric(*obj))
			count++;
		while (*obj && *obj != ' ' && *obj != '\n')
			obj++;
	}
	return (count);
}

static void	obj_link_face(t_obj *obj, t_face *f, char *d)
{
	int			index;
	int			ptIndex;

	index = -1;
	while (++index < MAX_FACE_POINT)
		f->pIndex[index] = 0;
	index = 0;
	while (*d && *d != '\n')
	{
		if (isNumeric(*d))
		{
			if ((ptIndex = atoi(d++)) > obj->num_points)
				continue;
			f->p[index] = &obj->p[ptIndex];
			f->pIndex[index] = ptIndex;
			if (++index >= MAX_FACE_POINT)
				break;
			while (*d && *d != ' ' && *d != '\n')
				d++;
		}
		else
			d++;
	}
}

int		obj_parse_faces(t_obj *obj, char *data)
{
	int		index;

	if (!obj || !data)
		return (1);
	if (obj->f)
	{
		free(obj->f);
		obj->f = NULL;
	}
	if ((obj->num_faces = count_verb(data, 'f')))
		if (!(obj->f = malloc(sizeof(struct s_face) * obj->num_faces)))
			return (1);
	index = 0;
	while (*data)
		if (*(data++) == '\n')
		{
			while (*data && (*data == ' ' || *data == '\n'))
				data++;
			if (data[0] == 'f' && (data[1] == ' ' || data[1] == '\t'))
			{
				obj->f[index].num_points = count_face_points(++data);
				obj_link_face(obj, &obj->f[index], data);
				index++;
			}
		}
	return (index != obj->num_faces ? 1 : 0);
}

t_obj		*new_obj(char *obj_data)
{
	t_obj	*obj;
	
	if (!(obj = malloc(sizeof(struct s_obj))))
		return (NULL);
	memset(obj, '\0', sizeof(struct s_obj));
	if (obj_parse_points(obj, obj_data) || obj_parse_faces(obj, obj_data))
		return (NULL);
	return (obj);
}

void		display_model(t_obj *obj)
{
	uint		i;

	i = -1;
	while (++i < obj->num_points)
		printf("#%d x[%f] y[%f] z[%f]\n", i, obj->p[i].x, obj->p[i].y, obj->p[i].z);
}

void 		export_model(t_obj *obj)
{
	int 		i;
	int		j;

	i = -1;
	while (++i < obj->num_points)
	{
		printf("v %f %f %f\n", obj->p[i].x, obj->p[i].y, obj->p[i].z);
	}
	i = -1;
	while (++i < obj->num_faces)
	{
		printf("f ");
		j = -1;
		while (++j < obj->f[i].num_points)
			printf("%d ", obj->f[i].pIndex[j]);
		printf("\n");
	}
}

static char	*decode_bitstream(char *bitstream)
{
	uint 		i;
	uint		j;
	int		length;
	char		*output;
	char		c;

	if ((length = strlen(bitstream)) <= 7 ||
		!(output = malloc(sizeof(char) * (length / 8) + 1)))
		return (NULL);
	memset(output, '\0', sizeof(char) * (length / 8) + 1);
	i = -1;
	j = 0;
	c = '\0';
	while (++i < length)
	{
		if (i != 0 && i % 8 == 0)
		{
			output[j++] = c;
			c = '\0';
		}
		c += bitstream[i] == '1' ? 1 : 0;
		if (i % 8 < 7)
			c <<= 1;
	}
	return (output);
}

char		*decode_lsb_obj(t_obj *obj)
{
	int		i;
	char		*output;
	char		*p;

	if (!(output = malloc(sizeof(char) * obj->num_points * 3)))
		return (NULL);
	p = output;
	i = -1;
	while (++i < obj->num_points)
	{
		*(p++) = (int)(obj->p[i].x * DECODE_EPSILON) % 2 != 0 ? '1' : '0';
		*(p++) = (int)(obj->p[i].y * DECODE_EPSILON) % 2 != 0 ? '1' : '0';
		*(p++) = (int)(obj->p[i].z * DECODE_EPSILON) % 2 != 0 ? '1' : '0';
	}
	*p = '\0';
	p = decode_bitstream(output);
	free(output);
	return (p);
}

char		*encode_bitstream(char *data, size_t size)
{
	size_t		i;
	size_t		j;
	char		*bitstream;
	char 		c;

	if (!(bitstream = malloc(sizeof(char) * size * 8 + 1)))
		return (NULL);
	memset(bitstream, '\0', sizeof(char) * size * 8 + 1);
	i = -1;
	j = 0;
	while (++i < size)
	{
		c = data[i];
		bitstream[j++] = c & 0x80 ? '1' : '0';
		bitstream[j++] = c & 0x40 ? '1' : '0';
		bitstream[j++] = c & 0x20 ? '1' : '0';
		bitstream[j++] = c & 0x10 ? '1' : '0';
		bitstream[j++] = c & 0x08 ? '1' : '0';
		bitstream[j++] = c & 0x04 ? '1' : '0';
		bitstream[j++] = c & 0x02 ? '1' : '0';
		bitstream[j++] = c & 0x01 ? '1' : '0';
	}
	return (bitstream);
}

void		encode_lsb_obj(t_obj *obj, char *data, size_t size)
{
	char		*bitstream;
	int		i;
	int		pt;
	int		length;

	if (!(bitstream = encode_bitstream(data, size)))
		return ;
	length = strlen(bitstream);
	i = -1;
	pt = -1;
	while (++i < length)
	{
		if (i % 3 == 0)
		{
			if (++pt >= obj->num_points)
				break;
			if ((int)(obj->p[pt].x * DECODE_EPSILON) % 2)
				obj->p[pt].x -= EPSILON;
			obj->p[pt].x += EPSILON * (bitstream[i] == '1' ? 1 : 0);
		}
		else if (i % 3 == 1)
		{
			if ((int)(obj->p[pt].y * DECODE_EPSILON) % 2)
				obj->p[pt].y -= EPSILON;
			obj->p[pt].y += EPSILON * (bitstream[i] == '1' ? 1 : 0);
		}
		else if (i % 3 == 2)
		{
			if ((int)(obj->p[pt].z * DECODE_EPSILON) % 2)
				obj->p[pt].z -= EPSILON;
			obj->p[pt].z += EPSILON * (bitstream[i] == '1' ? 1 : 0);
		}
	}
}

double			distance(struct s_point a, struct s_point b)
{
	double subx;
	double suby;
	double subz;
	double sum;

	subx = (b.x - a.x) * (b.x - a.x);
	suby = (b.y - a.y) * (b.y - a.y);
	subz = (b.z - a.z) * (b.z - a.z);
	sum = subx + suby + subz;
	if (sum == 0)
		return (0.0);
	return (sqrt(sum));
}

struct s_point sub_vec(struct s_point a, struct s_point b)
{
	struct s_point sub;

	sub.x = b.x - a.x;
	sub.y = b.y - a.y;
	sub.z = b.z - a.z;
	sub.w = 0;
	return (sub);
}

struct s_point	unit_vec(struct s_point a, struct s_point b)
{
	struct s_point unit;
	struct s_point v;
	double len;

	len = distance(a, b);
	v = sub_vec(a, b);
	unit.x = v.x / len;
	unit.y = v.y / len;
	unit.z = v.z / len;
	return (unit);
}

struct s_point mul_vec(struct s_point a, float value)
{
	struct s_point mul;

	mul.x = a.x * value;
	mul.y = a.y * value;
	mul.z = a.z * value;
	return (mul);
}

struct s_point add_vec(struct s_point a, struct s_point b)
{
	struct s_point	add;

	add.x = a.x + b.x;
	add.y = a.y + b.y;
	add.z = a.z + b.z;
	return (add);
}

void		display_vec(const char *prefix, struct s_point p)
{
	printf("%s x[%f] y[%f] z[%f]\n", prefix, p.x, p.y, p.z);//
}

char 		*decode_dist_lsb_obj(t_obj *obj, size_t *size)
{
	int 		ptIndex;
	t_point		*a;
	t_point		*b;
	char		*bitstream;
	char		*p;

	if (!(bitstream = malloc(sizeof(char) * obj->num_points - 1)))
		return (NULL);
	memset(bitstream, '\0', sizeof(char) * obj->num_points - 1);
	p = bitstream;
	ptIndex = -1;
	while (++ptIndex < obj->num_points - 1)
	{
		a = &obj->p[ptIndex];
		b = &obj->p[ptIndex + 1];

		double d;
		d = distance(*a, *b);
		*(p++) = (int)(d * DECODE_EPSILON) % 2 ? '1' : '0';
	}
	*p = '\0';
	if (size)
		*size = (size_t)(bitstream - p);
	p = decode_bitstream(bitstream);
	free(bitstream);
	return (p);
}

void		encode_dist_lsb_obj(t_obj *obj, char *data, size_t size)
{
	int 		ptIndex;
	t_point		*a;	
	t_point		*b;
	uint		dIndex;
	int		length;
	char		*bitstream;

	if (!(bitstream = encode_bitstream(data, size)))
		return ;
	length = strlen(bitstream);
	ptIndex = -1;
	dIndex = -1;
	while (++ptIndex < obj->num_points - 1)
	{
		dIndex++;
		if (dIndex >= size * 8)
			return ;
		a = &obj->p[ptIndex];
		b = &obj->p[ptIndex + 1];

		double d;
		d = distance(*a, *b);
		struct s_point unit;

		unit = unit_vec(*a, *b);
		unit = mul_vec(unit, EPSILON);
		if (bitstream[dIndex] == '1')
		{
			if ((int)(d * DECODE_EPSILON) % 2 == 0)
			{
				while ((int)(d * DECODE_EPSILON) % 2 == 0)
				{
					*b = add_vec(*b, unit);
					d = distance(*a, *b);
				}
			}
		}
		else if ((int)(d * DECODE_EPSILON) % 2 == 1)
		{
			unit = mul_vec(unit, -1.0);
			while ((int)(d * DECODE_EPSILON) % 2 == 1)
			{
				*b = add_vec(*b, unit);
				d = distance(*a, *b);
			}
		}
	}
}

int help()
{
	printf("Usage: <file.obj> [str]\n");
	return (0);
}

int main(int argc, char **argv)
{
	char		*obj;
	t_obj		*model;

	if (argc != 3 && argc != 2)
		return (help());
	if (!(obj = readalltext(argv[1], NULL)))
		return (printf("Null read\n"));
	//printf("obj: %s\n", obj);
	if (!(model = new_obj(obj)))
		return (printf("New obj error\n"));
	if (argc == 3)
	{
		if (METHOD_DIST)
			encode_dist_lsb_obj(model, argv[2], (size_t)strlen(argv[2]) + 2);
		else
			encode_lsb_obj(model, argv[2], (size_t)strlen(argv[2]) + 2);
		export_model(model);
		//printf("Decoded: [%s]\n", decode_lsb_obj(model));
		//printf("Decoded: [%s]\n", decode_dist_lsb_obj(model, NULL));
	}
	else if (argc == 2)
	{
		if (METHOD_DIST)
			printf("Decoded: [%s]\n", decode_dist_lsb_obj(model, NULL));
		else
			printf("Decoded: [%s]\n", decode_lsb_obj(model));
	}
	return (0);
}
