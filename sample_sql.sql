DROP TABLE ORDERS;
DROP TABLE PAY;
DROP TABLE CUSTOMER;

CREATE TABLE ORDERS (order_id INTEGER PRIMARY KEY, title VARCHAR(30),price Integer);
CREATE TABLE Pay (cust_id Integer, order_id Integer);
CREATE TABLE Customer (cust_id Integer PRIMARY KEY, name varchar(30));
INSERT INTO orders values(1,'Big Data',30);
INSERT INTO orders values(2,'SQL',35);
INSERT INTO orders values(3,'Logic',50);
INSERT INTO pay values(1,1);
INSERT INTO pay values(2,NULL);
INSERT INTO customer values(1,'John');
INSERT INTO customer values(2,'Mary');
load 'certain_answer_codd';

\echo 'Relations: '
SELECT * FROM orders;
SELECT * FROM pay;
SELECT * FROM customer;

\echo 'The query: SELECT O.order_id FROM orders O WHERE NOT EXISTS( SELECT * FROM pay WHERE O.order_id = pay.order_id);\n'


\echo 'Default behaviour'
SET certain_answer_codd.certain to 0;
SELECT O.order_id FROM orders O WHERE NOT EXISTS( SELECT * FROM pay WHERE O.order_id = pay.order_id);

\echo 'Certain answers'
SET certain_answer_codd.certain to 1;
SELECT O.order_id FROM orders O WHERE NOT EXISTS( SELECT * FROM pay WHERE O.order_id = pay.order_id);

\echo 'Possible answers'
SET certain_answer_codd.certain to 2;
SELECT O.order_id FROM orders O WHERE NOT EXISTS( SELECT * FROM pay WHERE O.order_id = pay.order_id);

\echo 'The query: SELECT c.cust_id FROM customer C WHERE NOT EXISTS (SELECT * FROM Orders,Pay WHERE C.cust_id = Pay.cust_id AND Pay.order_id = Orders.order_id);\n'

\echo 'Default behaviour'
SET certain_answer_codd.certain to 0;
SELECT c.cust_id FROM customer C WHERE NOT EXISTS (SELECT * FROM Orders,Pay WHERE C.cust_id = Pay.cust_id AND Pay.order_id = Orders.order_id);

\echo 'Certain answers'
SET certain_answer_codd.certain to 1;
SELECT c.cust_id FROM customer C WHERE NOT EXISTS (SELECT * FROM Orders,Pay WHERE C.cust_id = Pay.cust_id AND Pay.order_id = Orders.order_id);

\echo 'Possible answers'
SET certain_answer_codd.certain to 2;
SELECT c.cust_id FROM customer C WHERE NOT EXISTS (SELECT * FROM Orders,Pay WHERE C.cust_id = Pay.cust_id AND Pay.order_id = Orders.order_id);

