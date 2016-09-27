<?php
require_once "recaptcha_verify.php";

$to = 'maturabel@gmail.com';
$subject = 'формуляр';
$first_name = $_POST['first_name'];
$class_number = $_POST['class_number'];
$class_letter = $_POST['class_letter'];
$satisfied = $_POST['satisfied'];
$comment = $_POST['comment'];
$message = <<<EMAIL
Подовател: $first_name
От: $class_number $class_letter
Подавателят е доволен: $satisfied
Допълнителен коментар:
-$comment
EMAIL;

if($_POST)
{
	if($class_number == '' || $class_letter == '' || $satisfied == '')
	{
		$feedback = 'Моля попълнете полетата.';
	}
	else
	{

		if( recaptcha_verify( $_POST["g-recaptcha-response"] ) )
		{
			$feedback = "Благодарим ви много.";
			mail($to, $subject, $message);
		}
		
		else
		{
			$feedback = "Моля въведете recaptcha кода.";
		}
	}
}
?>
<!DOCTYPE html>
<html>
	<head>
		<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
		<title>Обратна връзка</title>
		<link href="images/favicon.ico" rel="icon" type="image/gif" sizes="32x32">
		<link href="http://fonts.googleapis.com/css?family=Arizonia" rel="stylesheet" type="text/css" />
		<link href="css/styles.css" rel="stylesheet" type="text/css" />
		<link href="css/obratna_vruzka.css" rel="stylesheet" type="text/css"/>
		<script src='https://www.google.com/recaptcha/api.js?hl=bg'></script>
	</head>
	<body>
		<div id="wrapper">
			<!--navigation bar-->
			<div id="nav_bar">
				<a href="nachalo.html" id="logo">maturabel</a>
				<ul>
					<li class="button"><a href="nachalo.html">Начало</a></li>
					<li class="button"><a href="format-na-izpita.html">Формат на изпита</a></li>
					<li class="button dropdown"><a>Български</a><img src="images/dropdown.png" />
						<div class="dropdown_content">
							<a href="pravopis.html">Правопис</a>
							<a href="gramatika.html">Граматика</a>
							<a href="punktuaciq.html">Пунктуация</a>
							<a href="leksika.html">Лексика</a>
							<a href="sferi-na-obshtuvane.html">Сфери на общуване</a>
						</div>
					</li>
					<li class="button dropdown"><a>Литература</a><img src="images/dropdown.png" />
						<div class="dropdown_content">
							<a href="literatura-predgovor.html">Предговор</a>
							<a href="za-avtorite.html">За авторите</a>
							<a href="izrazni-sredstva.html">Изразни средства</a>
							<a href="terminologiq.html">Терминология</a>
							<a href="zhanrove.html">Жанрове</a>
							<a href="teza-i-rezyume.html">Теза и резюме</a>
							<a href="ese-i-lis.html">Есе и ЛИС</a>
						</div>
					</li>
					<li class="button active"><a href="obratna-vruzka.php">Обратна връзка</a></li>
				</ul>
			</div>
			<!--content-->
			<div id="container">
			<h1>Обратна връзка</h1>
			<p>
				Моля, ако ви харесва този уебсайт и написаното тук, попълнете тази кратка анкета. Ако не ви харесва, с удоволствие
				бихме изслушали вашата критика. Попълването няма да отнеме повече от минута.
			</p>
			<p style="margin-top: 10px; font-weight: bold;"><?php echo $feedback; ?>
			</p>
			<form method="post">
				<div class="field" style="margin-top: 5px;">
					<span class="field_name">Вашето име <span style="font-weight: normal;">(това поле не е задължително)</span></span><br/>
					<input type="text" name="first_name" id="first_name" placeholder="Собствено">
				</div>
				<div class="field">
					<span class="field_name">Клас: </span><br/>
					<select name="class_number" id="class_number">
						<option selected disabled></option>
						<option value="11">11</option>
						<option value="12">12</option>
					</select>
					<select name="class_letter" id="class_letter">
						<option selected disabled></option>
						<option value="А">А</option>
						<option value="Б">Б</option>
						<option value="В">В</option>
						<option value="Г">Г</option>
						<option value="Д">Д</option>
						<option value="Е">Е</option>
					</select>
				</div>
				<div class="field">
					<span class="field_name">Доволни ли сте от този сайт?</span>
					<input type="radio" name="satisfied" id="satisfied" value="yes"><span style="font-family: sans-serif;font-size: 14px;"> Да</span><br/>
					<input type="radio" name="satisfied" id="satisfied" value="no"><span style="font-family: sans-serif;font-size: 14px;"> Не</span>
				</div>
				<div class="field">
					<span class="field_name">Коментар <span style="font-weight: normal;">(това поле не е задължително)</span></span>
					<textarea name="comment" name="comment"></textarea>
				</div>
				<!--captcha-->
				<div style="margin-top: 15px; margin-bottom: 15px;" class="g-recaptcha" data-sitekey="6LfoGh0TAAAAADHwhDAVs5sj_uv3sVCexPK3zpcL"></div>
				<!--captcha-->
				<input type="submit" value="Пращане">
				<input type="reset" value="Изтрий">
			</form>
			</div>
			<!--footer-->
			<div id="footer">
				<span style="padding-left: 4%;">Съдържанието тук е общодостъпно с <a href="pravila-za-polzvane.html" style="color: #505050;">някои ограничения</a></span>
				<span style="padding-left: 4%;">Ако смятате, че вашите авторски права са нарушени, моля свържете се с нас: maturabel@gmail.com
			</div>
		</div>
	</body>
</html>
