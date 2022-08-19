#include "connection.h"
#include "service.h"

// 将字符串p1data_str以字符串p1split_str截断
// 前半部分放入p1front_str，剩余部分放入p1data_str
char *str_split(char *p1data_str, char *p1split_str, char *p1front_str);

int recv_data(connection *p1conn) {
  if (0 == p1conn->connfd) {
    // 无效的client的socket句柄
    return -1;
  }
  if (1 == p1conn->recv_buffer_full) {
    // 接收缓冲区满了，则直接返回0
    return 0;
  }
  // 计算剩余空间长度
  int remain_len = p1conn->recv_buffer_max - p1conn->recv_buffer_last;
  if (remain_len > 0) {
    // 接收缓冲区还有空间，接收数据
    // 计算剩余空间的第1个字节的内存地址
    char *temp_index = p1conn->p1recv_buffer + p1conn->recv_buffer_last;
    ssize_t recv_bytes = -1;
    recv_bytes = recv(p1conn->connfd, temp_index, remain_len, 0);
    if (is_debug() == 1) {
      printf("[debug]:recv_data(),recv_bytes=%d\r\n", recv_bytes);
    }
    if (-1 == recv_bytes) {
      printf("[error]:recv_data(),-1==recv_bytes");
      printf("[error]:errno=%d,errstr%s\r\n", errno, strerror(errno));
      return -1;
    }
    if (0 == recv_bytes) {
      // 测试一下对端是不是已关闭
      ssize_t send_bytes = -1;
      send_bytes = send(p1conn->connfd, "", 0, 0);
      if (-1 == send_bytes || 0 == send_bytes) {
        // 对端已关闭，返回异常
        printf("[error]:recv_data(),-1==recv_bytes");
        printf("[error]:errno=%d,errstr%s\r\n", errno, strerror(errno));
        return -1;
      } else {
        // 真的没接收到数据
        if (is_debug() == 1) {
          printf("[debug]:recv_data(),send(),send_bytes=%d\r\n", send_bytes);
        }
        return -1;
      }
    } else {
      // 收到数据，返回本次接收到的数据长度
      p1conn->recv_buffer_last += recv_bytes;
      // recv_buffer末尾要加`\0`
      p1conn->p1recv_buffer[p1conn->recv_buffer_last] = '\0';

      return recv_bytes;
    }
  } else {
    // 接收缓冲区满了，标记一下，然后返回0
    p1conn->recv_buffer_full = 1;

    return 0;
  }
};

int push_data(connection *p1conn, char *res_data, int res_data_len) {
  if (0 == p1conn->connfd) {
    return -1;
  }
  if (1 == p1conn->send_buffer_full) {
    return 0;
  }
  int remain_len = p1conn->send_buffer_max - p1conn->send_buffer_last;
  if (res_data_len <= remain_len) {
    char *temp_index = p1conn->p1send_buffer + p1conn->send_buffer_last;
    memcpy(temp_index, res_data, res_data_len);
    p1conn->send_buffer_last += res_data_len;

    return res_data_len;
  } else {
    p1conn->send_buffer_full = 1;

    return 0;
  }
};

int write_data(connection *p1conn) {
  if (0 == p1conn->connfd) {
    return -1;
  }

  if (is_debug() == 1) {
    printf("[debug]:write_data(),p1conn->connfd=%d\r\n", p1conn->connfd);
  }
  if (p1conn->send_buffer_last > 0) {
    // 发送缓冲区有数据才发
    int send_bytes = -1;
    if (is_debug() == 1) {
      printf("[debug]:write_data(),p1conn->send_buffer_last=%d\r\n", p1conn->send_buffer_last);
      // 这里当第1次发送的数据比第2次多时，输出的时候会有第1次的数据
      // 但是发送数据是按send_buffer_last发送的，所以不会出问题
      printf("[debug]:write_data(),send(),p1conn->send_buffer=\r\n%s\r\n", p1conn->p1send_buffer);
    }
    send_bytes = send(p1conn->connfd, p1conn->p1send_buffer, p1conn->send_buffer_last, 0);
    if (is_debug() == 1) {
      printf("[debug]:write_data(),send(),send_bytes=%d\r\n", send_bytes);
    }
    if (-1 == send_bytes || 0 == send_bytes) {
      printf("[error]:write_data(),-1==send_bytes,0==send_bytes");
      printf("[error]:errno=%d,errstr%s\r\n", errno, strerror(errno));
      return -1;
    }
    if (send_bytes == p1conn->send_buffer_last) {
      //一次性发送完，直接重置发送缓冲区
      memset(p1conn->p1send_buffer, 0, sizeof(p1conn->p1send_buffer));
      p1conn->send_buffer_last = 0;
      p1conn->send_buffer_full = 0;

      return 0;
    } else {
      // 只发了一半，没发送完
      // 发出去的是发送缓冲区前面的数据
      // 计算发送缓冲区还有多少数据没有发送出去
      p1conn->send_buffer_last -= send_bytes;
      // 把后面的数据移到前面覆盖前面已经发送过的数据
      memcpy(p1conn->p1send_buffer, p1conn->p1send_buffer + send_bytes, p1conn->send_buffer_last);

      return 1;
    }
  }

  return 0;
};

int get_http_req_complete(connection *p1conn) {
  if (0 == p1conn->connfd) {
    return -1;
  }

  // 48 POST /hello?queue_name=queue_kelipute HTTP/1.1\r\n
  // 53 Postman-Token: 61e22a75-09e0-4d13-b532-c861d8b44629\r\n
  // 24 Connection: keep-alive\r\n
  // 55 Content-Type: application/x-www-form-urlencoded\r\n
  // 20 Content-Length: 23\r\n
  // 2 \r\n
  // 23 form_name=form_kelipute

  // 找到请求头和请求体之间的`\r\n\r\n`的位置
  char *p1rnrn = strstr(p1conn->p1recv_buffer, "\r\n\r\n");
  if (NULL == p1rnrn) {
    return -1;
  }
  int header_len = 0;
  // 通过`\r\n\r\n`的位置，计算http报文请求行和请求头的长度
  header_len = p1rnrn - p1conn->p1recv_buffer;
  // 再加上`\r\n\r\n`子串的长度4，就是完整的http报文请求行和请求头的长度
  header_len += 4;
  int body_len = 0;
  // 通过请求头的`Content-Length: `字段，判断有没有请求体
  char *p1content_length_index = strstr(p1conn->p1recv_buffer, "Content-Length: ");
  if (p1content_length_index != NULL) {
    // 找到`Content-Length: `字段后面一个`\r\n`的位置
    char *p1content_length_rn_index = strstr(p1content_length_index, "\r\n");
    // 计算`Content-Length: `字段后面数字字符串的长度，字符串`Content-Length: `的长度是16
    int content_length = p1content_length_rn_index - (p1content_length_index + 16);
    if (content_length > 7) {
      // 数字字符串的长度大于7，表示数据超过1MB，1024*1024=1048576
      return -1;
    }
    // 数字字符串转换成数字
    char arr1content_length[7];
    strncpy(arr1content_length, p1content_length_index + 16, content_length);
    body_len = atoi(arr1content_length);
    if (header_len + body_len > p1conn->recv_buffer_last) {
      // 消息没接收完整
      return -1;
    }
    if (header_len + body_len > p1conn->recv_buffer_max) {
      // 数据超过1MB
      return -1;
    }
  }

  if (is_debug() == 1) {
    printf("[debug]:get_http_req_complete(),header_len=%d,body_len=%d\r\n", header_len, body_len);
  }

  p1conn->p1http_data = (http_data *)malloc(sizeof(http_data));
  p1conn->p1http_data->p1method = NULL;
  p1conn->p1http_data->p1uri = NULL;
  p1conn->p1http_data->p1version = NULL;
  p1conn->p1http_data->header_len = header_len;
  p1conn->p1http_data->p1header_data = NULL;
  p1conn->p1http_data->body_len = body_len;
  p1conn->p1http_data->p1get_data = NULL;
  p1conn->p1http_data->p1post_data = NULL;
  return 0;
}

int parse_http_req(connection *p1conn) {
  if (p1conn->p1http_data->header_len <= 0) {
    // 无效数据
    return -1;
  }

  char *p1http_data_str = p1conn->p1recv_buffer;

  // 把`\r\n\r\n`改成`\r\n\r\0`
  p1http_data_str[p1conn->p1http_data->header_len - 1] = '\0';

  // 解析请求行
  // 把请求行的`\r\n`改成`\0\n`
  char *p1temp_rn = strstr(p1http_data_str, "\r\n");
  p1temp_rn[0] = '\0';

  char arr1method[16];
  char arr1uri[128];
  char arr1version[16];
  p1http_data_str = str_split(p1http_data_str, " ", arr1method);
  p1http_data_str = str_split(p1http_data_str, " ", arr1uri);
  p1http_data_str = str_split(p1http_data_str, " ", arr1version);
  if (is_debug() == 1) {
    printf("[debug]:parse_http_req(),arr1method=%s,arr1uri=%s,arr1version=%s\r\n", arr1method, arr1uri, arr1version);
  }

  p1conn->p1http_data->p1method = (char *)malloc(sizeof(char) * sizeof(arr1method));
  strcpy(p1conn->p1http_data->p1method, arr1method);
  p1conn->p1http_data->p1uri = (char *)malloc(sizeof(char) * sizeof(arr1uri));
  strcpy(p1conn->p1http_data->p1uri, arr1uri);
  p1conn->p1http_data->p1version = (char *)malloc(sizeof(char) * sizeof(arr1version));
  strcpy(p1conn->p1http_data->p1version, arr1version);

  // 解析查询字符串
  char *p1query;
  p1query = str_split(arr1uri, "?", arr1uri);
  // 更新uri字段
  strcpy(p1conn->p1http_data->p1uri, arr1uri);
  if (is_debug() == 1) {
    printf("[debug]:parse_http_req(),arr1uri=%s,arr1query=%s\r\n", arr1uri, p1query);
  }
  if (p1query != "") {
    p1conn->p1http_data->p1get_data = (data_kv *)malloc(sizeof(data_kv) * 16);
    int get_data_index = 0;
    while (1) {
      char arr1query_kv[128];
      p1query = str_split(p1query, "&", arr1query_kv);
      if (arr1query_kv != "") {
        char arr1key[128];
        char *p1value;
        p1value = str_split(arr1query_kv, "=", arr1key);
        if (is_debug() == 1) {
          printf("[debug]:parse_http_req(),query_key=%s,query_value=%s\r\n", arr1key, p1value);
        }
        data_kv temp_data_kv;
        temp_data_kv.p1key = (char *)malloc(sizeof(char) * strlen(arr1key));
        temp_data_kv.p1value = (char *)malloc(sizeof(char) * strlen(p1value));
        strcpy(temp_data_kv.p1key, arr1key);
        strcpy(temp_data_kv.p1value, p1value);
        p1conn->p1http_data->p1get_data[get_data_index] = temp_data_kv;
        get_data_index++;
      }
      if (16 == get_data_index || "" == p1query) {
        break;
      }
    }
  }

  // 解析请求头
  p1temp_rn += 2;
  char *p1header_str = p1temp_rn;

  p1conn->p1http_data->p1header_data = (data_kv *)malloc(sizeof(data_kv) * 16);
  int header_data_index = 0;
  while (1) {
    char arr1header[128];
    p1header_str = str_split(p1header_str, "\r\n", arr1header);

    if (strcmp("\0\r", p1header_str) != 0) {
      char arr1key[128];
      char *p1value;
      p1value = str_split(arr1header, ": ", arr1key);
      if (is_debug() == 1) {
        printf("[debug]:parse_http_req(),header_key=%s,header_value=%s\r\n", arr1key, p1value);
      }
      data_kv temp_data_kv;
      temp_data_kv.p1key = (char *)malloc(sizeof(char) * strlen(arr1key));
      temp_data_kv.p1value = (char *)malloc(sizeof(char) * strlen(p1value));
      strcpy(temp_data_kv.p1key, arr1key);
      strcpy(temp_data_kv.p1value, p1value);
      // 解析过的位置，设置无效
      memset(arr1header, 0, sizeof(arr1header));
      p1conn->p1http_data->p1header_data[header_data_index] = temp_data_kv;
      header_data_index++;
      if (16 == header_data_index) {
        break;
      }
    }
    if ("" == p1header_str) {
      break;
    }
  }

  // 解析请求体
  if (p1conn->p1http_data->body_len > 0) {
    char *p1body_str = p1conn->p1recv_buffer + p1conn->p1http_data->header_len;
    p1conn->p1http_data->p1post_data = (data_kv *)malloc(sizeof(data_kv) * 16);
    int post_data_index = 0;
    while (1) {
      char arr1post[128];
      p1body_str = str_split(p1body_str, "&", arr1post);
      if (arr1post != "") {
        char arr1key[128];
        char *p1value;
        p1value = str_split(arr1post, "=", arr1key);
        if (is_debug() == 1) {
          printf("[debug]:parse_http_req(),post_key=%s,post_value=%s\r\n", arr1key, p1value);
        }
        data_kv temp_data_kv;
        temp_data_kv.p1key = (char *)malloc(sizeof(char) * strlen(arr1key));
        temp_data_kv.p1value = (char *)malloc(sizeof(char) * strlen(p1value));
        strcpy(temp_data_kv.p1key, arr1key);
        strcpy(temp_data_kv.p1value, p1value);
        p1conn->p1http_data->p1post_data[post_data_index] = temp_data_kv;
        post_data_index++;
        if (16 == post_data_index) {
          break;
        }
      }
      if ("" == p1body_str) {
        break;
      }
    }
  }

  return 0;
}

char *str_split(char *p1data_str, char *p1split_str, char *p1front_str) {
  // 找出split_str在p1data_str中第一次出现的位置
  char *p1temp_index = strstr(p1data_str, p1split_str);
  if (p1temp_index == NULL) {
    // 如果没找到p1split_str，就把p1data_str全部复制给p1front_str
    if (p1front_str != NULL) {
      strcpy(p1front_str, p1data_str);
    }
    // 将p1data_str赋值为空字符串
    return "";
  } else {
    // 如果找到p1split_str，就把p1split_str第一个字符替换成'\0'
    // 这样在strcpy()的时候，只会把'\0'前面的数据复制给p1front_str
    p1temp_index[0] = '\0';
    if (p1front_str != NULL) {
      strcpy(p1front_str, p1data_str);
    }
    // 移动p1data_str的位置到p1split_str后面
    return p1temp_index + strlen(p1split_str);
  }
}

char *get_header(connection *p1conn, char *p1key) {
  if (NULL == p1conn->p1http_data->p1header_data) {
    return "";
  }
  int i = 0;
  int key_len = strlen(p1key);
  while (p1conn->p1http_data->p1header_data[i].p1key != '\0') {
    if (strcmp(p1conn->p1http_data->p1header_data[i].p1key, p1key) == 0) {
      return p1conn->p1http_data->p1header_data[i].p1value;
    }
    if (i > key_len) {
      break;
    }
    i++;
  }
  return "";
}

char *get_query(connection *p1conn, char *p1key) {
  if (NULL == p1conn->p1http_data->p1get_data) {
    return "";
  }
  int i = 0;
  int key_len = strlen(p1key);
  while (p1conn->p1http_data->p1get_data[i].p1key != '\0') {
    if (strcmp(p1conn->p1http_data->p1get_data[i].p1key, p1key) == 0) {
      return p1conn->p1http_data->p1get_data[i].p1value;
    }
    if (i > key_len) {
      break;
    }
    i++;
  }
  return "";
}

char *get_post(connection *p1conn, char *p1key) {
  if (NULL == p1conn->p1http_data->p1post_data) {
    return "";
  }
  int i = 0;
  int key_len = strlen(p1key);
  while (p1conn->p1http_data->p1post_data[i].p1key != '\0') {
    if (strcmp(p1conn->p1http_data->p1post_data[i].p1key, p1key) == 0) {
      return p1conn->p1http_data->p1post_data[i].p1value;
    }
    if (i > key_len) {
      break;
    }
    i++;
  }
  return "";
}

void clear_recv_buffer(connection *p1conn) {
  if (p1conn->recv_buffer_last > 0) {
    // 计算这次处理的报文长度和接收缓冲区剩下的数据的长度
    int content_len = p1conn->p1http_data->header_len + p1conn->p1http_data->body_len;
    int remain_len = p1conn->recv_buffer_last - content_len;
    if (remain_len > 0) {
      memcpy(p1conn->p1recv_buffer, p1conn->p1recv_buffer + content_len, remain_len);
    }
    p1conn->recv_buffer_last = remain_len;

    if (1 == p1conn->recv_buffer_full) {
      p1conn->recv_buffer_full = 0;
    }
  }
}